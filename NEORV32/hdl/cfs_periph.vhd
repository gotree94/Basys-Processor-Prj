-- ================================================================================
-- cfs_periph.vhd - NEORV32 CFS (Custom Functions Subsystem) replacement
-- -------------------------------------------------------------------------------
-- neorv32_top 안의 <neorv32_cfs> 인스턴스를 대체하는 설계. 엔티티 이름/포트는
-- 업스트림 neorv32_cfs.vhd 와 완전히 동일해야 한다 (Vivado에서는 업스트림
-- neorv32_cfs.vhd 파일은 프로젝트에 포함하지 말고 이 파일만 포함할 것).
--
-- 메모리맵 (CFS base + 오프셋, byte addressing):
--   0x0000(wa=0)    SEG7_DIG0    w      digits[3:0]
--   0x0004(wa=1)    SEG7_DIG1    w      digits[7:4]
--   0x0008(wa=2)    SEG7_DIG2    w      digits[11:8]
--   0x000C(wa=3)    SEG7_DIG3    w      digits[15:12]
--   0x0010(wa=4)    SEG7_RAW     w      raw 8bit (a..g,dp) 디코더 우회
--   0x0100(wa=0x40) VGA_CTRL     r/w    [0]=enable
--   0x0104(wa=0x41) VGA_STATUS   r      [0]=vblank
--   0x0200(wa=0x80) PS2_DATA     r      [8]=valid [7:0]=scancode (읽으면 pop)
--   0x0204(wa=0x81) PS2_CTRL     r/w    [0]=irq_en
--   0x0500(wa=0x140)~0x14FC      VGA 프레임버퍼 4800 words x 32bit
--                                 (160x120x8bpp, 4px/word)
--
-- conduit (cfs_out_o): [6:0]=seg(cat) [7]=dp [11:8]=an(active low)
--                       [15:12]=vgaR [19:16]=vgaG [23:20]=vgaB
--                       [24]=Hsync [25]=Vsync
-- conduit (cfs_in_i):  [0]=PS2Clk [1]=PS2Data
-- ================================================================================

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity neorv32_cfs is
  port (
    clk_i      : in  std_ulogic;
    rstn_i     : in  std_ulogic;
    req_addr_i : in  std_ulogic_vector(15 downto 0);
    req_data_i : in  std_ulogic_vector(31 downto 0);
    req_ben_i  : in  std_ulogic_vector(3 downto 0);
    req_stb_i  : in  std_ulogic;
    req_rw_i   : in  std_ulogic;
    rsp_data_o : out std_ulogic_vector(31 downto 0);
    rsp_ack_o  : out std_ulogic;
    irq_o      : out std_ulogic;
    cfs_in_i   : in  std_ulogic_vector(255 downto 0);
    cfs_out_o  : out std_ulogic_vector(255 downto 0)
  );
end entity;

architecture rtl of neorv32_cfs is

  constant FB_X        : natural := 160;      -- framebuffer width
  constant FB_Y        : natural := 120;      -- framebuffer height
  constant FB_WORDS    : natural := (FB_X/4) * FB_Y;              -- 4800
  constant FB_OFFSET_W : natural := 16#140#;  -- FB word index base in CFS space

  -- registers
  signal digits_s  : std_ulogic_vector(15 downto 0);
  signal raw_s     : std_ulogic_vector(7 downto 0);
  signal raw_en_s  : std_ulogic;
  signal vga_en_s  : std_ulogic;
  signal ps2_irq_s : std_ulogic;

  -- framebuffer (port A = CPU, port B = VGA)
  signal fb_addr_s   : std_ulogic_vector(12 downto 0);  -- CPU-side word addr
  signal fb_wen_s    : std_ulogic;
  signal fb_wbe_s    : std_ulogic_vector(3 downto 0);
  signal fb_wdata_s  : std_ulogic_vector(31 downto 0);
  signal fb_rdata_s  : std_ulogic_vector(31 downto 0);
  signal fb_pen_s    : std_ulogic;             -- pending CPU-FB transaction
  signal fb_iswr_s   : std_ulogic;

  -- 7-seg
  signal cat_s     : std_ulogic_vector(6 downto 0);
  signal dp_s      : std_ulogic;
  signal an_s      : std_ulogic_vector(3 downto 0);

  -- VGA
  signal vga_r_s, vga_g_s, vga_b_s : std_ulogic_vector(3 downto 0);
  signal hsync_s, vsync_s, vblank_s : std_ulogic;
  signal vga_addr_s : std_ulogic_vector(12 downto 0);
  signal vga_word_s : std_ulogic_vector(31 downto 0);

  -- PS/2
  signal ps2_rx_data_s  : std_ulogic_vector(7 downto 0);
  signal ps2_rx_valid_s : std_ulogic;
  type fifo_t is array (0 to 7) of std_ulogic_vector(7 downto 0);
  signal fifo_s   : fifo_t := (others => (others => '0'));
  signal fifo_n_s : integer range 0 to 8 := 0;

  -- decoded flags (combinational)
  signal is_fb_s : std_ulogic;
  signal wa_s    : unsigned(15 downto 0);

begin

  -----------------------------------------------------------------------------
  -- Address decode
  -----------------------------------------------------------------------------
  wa_s     <= unsigned(req_addr_i(15 downto 2));
  is_fb_s  <= '1' when (wa_s >= FB_OFFSET_W) and (wa_s < (FB_OFFSET_W + FB_WORDS))
              else '0';

  -----------------------------------------------------------------------------
  -- CPU access / response (register + FIFO)
  -----------------------------------------------------------------------------
  access: process(clk_i, rstn_i)
    variable wa : integer;
  begin
    if (rstn_i = '0') then
      rsp_ack_o   <= '0';
      digits_s    <= (others => '0');
      raw_s       <= (others => '0');
      raw_en_s    <= '0';
      vga_en_s    <= '0';
      ps2_irq_s   <= '0';
      fb_pen_s    <= '0';
      fb_iswr_s   <= '0';
      fb_addr_s   <= (others => '0');
      fb_wen_s    <= '0';
      fb_wbe_s    <= (others => '0');
      fb_wdata_s  <= (others => '0');
      fifo_n_s    <= 0;
      fifo_s      <= (others => (others => '0'));
    elsif rising_edge(clk_i) then
      rsp_ack_o <= '0';
      fb_wen_s  <= '0';

      -- pending FB transaction (one-cycle later ACK)
      if (fb_pen_s = '1') then
        rsp_ack_o <= '1';
        fb_wen_s  <= fb_iswr_s;
        fb_pen_s  <= '0';
      end if;

      -- new bus access
      if (req_stb_i = '1') then
        wa := to_integer(wa_s);
        if (is_fb_s = '1') then
          -- framebuffer access (write or read), acknowledged one cycle later
          fb_pen_s   <= '1';
          fb_iswr_s  <= req_rw_i;
          fb_addr_s  <= std_ulogic_vector(to_unsigned(wa - FB_OFFSET_W, 13));
          fb_wdata_s <= req_data_i;
          fb_wbe_s   <= req_ben_i;
        elsif (req_rw_i = '1') then
          -- register writes
          case wa is
            when 0      => digits_s(3 downto 0)  <= req_data_i(3 downto 0);
            when 1      => digits_s(7 downto 4)  <= req_data_i(3 downto 0);
            when 2      => digits_s(11 downto 8) <= req_data_i(3 downto 0);
            when 3      => digits_s(15 downto 12)<= req_data_i(3 downto 0);
            when 4      => raw_s <= req_data_i(7 downto 0); raw_en_s <= '1';
            when 16#40# => vga_en_s <= req_data_i(0);
            when 16#81# => ps2_irq_s <= req_data_i(0);
            when others => null;
          end case;
          rsp_ack_o <= '1';
        else
          -- register reads (combinational data, see combi process)
          rsp_ack_o <= '1';
          if (wa = 16#80#) and (fifo_n_s > 0) then  -- pop
            for i in 0 to 6 loop
              fifo_s(i) <= fifo_s(i+1);
            end loop;
            fifo_n_s <= fifo_n_s - 1;
          end if;
        end if;
      end if;

      -- PS/2 FIFO push
      if (ps2_rx_valid_s = '1') and (fifo_n_s < 8) then
        fifo_s(fifo_n_s) <= ps2_rx_data_s;
        fifo_n_s         <= fifo_n_s + 1;
      end if;

    end if;
  end process access;

  -----------------------------------------------------------------------------
  -- read-data mux (combinational)
  -----------------------------------------------------------------------------
  read_mux: process(wa_s, digits_s, raw_s, vga_en_s, vblank_s, fifo_s, fifo_n_s,
                    ps2_irq_s, fb_pen_s, fb_rdata_s)
    variable wa : integer;
  begin
    rsp_data_o <= (others => '0');
    if (fb_pen_s = '1') then
      rsp_data_o <= fb_rdata_s;              -- FB read result (registered RAM)
    else
      wa := to_integer(wa_s);
      case wa is
        when 0      => rsp_data_o <= x"0000000" & "0000" & digits_s(3 downto 0);
        when 1      => rsp_data_o <= x"0000000" & "0000" & digits_s(7 downto 4);
        when 2      => rsp_data_o <= x"0000000" & "0000" & digits_s(11 downto 8);
        when 3      => rsp_data_o <= x"0000000" & "0000" & digits_s(15 downto 12);
        when 4      => rsp_data_o <= x"000000" & raw_s;
        when 16#40# => rsp_data_o(0) <= vga_en_s;
        when 16#41# => rsp_data_o(0) <= vblank_s;
        when 16#80# => rsp_data_o(8) <= '1' when (fifo_n_s > 0) else '0';
                       rsp_data_o(7 downto 0) <= fifo_s(0);
        when 16#81# => rsp_data_o(0) <= ps2_irq_s;
        when others => null;
      end case;
    end if;
  end process read_mux;

  -----------------------------------------------------------------------------
  -- framebuffer RAM (TDP: A=CPU r/w, B=VGA read)
  -----------------------------------------------------------------------------
  framebuffer_inst: entity work.fb_ram
    generic map (FB_WORDS => FB_WORDS)
    port map (
      clk_i      => clk_i,
      a_addr_i   => fb_addr_s,
      a_wen_i    => fb_wen_s,
      a_wbe_i    => fb_wbe_s,
      a_wdata_i  => fb_wdata_s,
      a_rdata_o  => fb_rdata_s,
      b_addr_i   => vga_addr_s,
      b_rdata_o  => vga_word_s
    );

  -----------------------------------------------------------------------------
  -- 7-seg controller (4 digits, hex decoder + refresh)
  -----------------------------------------------------------------------------
  seg7_inst: entity work.seg7_ctrl
    port map (
      clk_i     => clk_i,
      rstn_i    => rstn_i,
      digits_i  => digits_s,
      raw_i     => raw_s,
      raw_en_i  => raw_en_s,
      cat_o     => cat_s,
      dp_o      => dp_s,
      an_o      => an_s
    );

  -----------------------------------------------------------------------------
  -- VGA controller (640x480@60, 25MHz pixel clock from 100MHz / 4)
  -----------------------------------------------------------------------------
  vga_inst: entity work.vga_ctrl
    generic map (
      FB_X   => FB_X,
      FB_Y   => FB_Y,
      FB_WORDS => FB_WORDS
    )
    port map (
      clk_i      => clk_i,
      rstn_i     => rstn_i,
      en_i       => vga_en_s,
      fb_addr_o  => vga_addr_s,
      fb_word_i  => vga_word_s,
      r_o        => vga_r_s,
      g_o        => vga_g_s,
      b_o        => vga_b_s,
      hsync_o    => hsync_s,
      vsync_o    => vsync_s,
      vblank_o   => vblank_s
    );

  -----------------------------------------------------------------------------
  -- PS/2 receiver
  -----------------------------------------------------------------------------
  ps2_inst: entity work.ps2_rx
    port map (
      clk_i      => clk_i,
      rstn_i     => rstn_i,
      ps2_clk_i  => cfs_in_i(0),
      ps2_dat_i  => cfs_in_i(1),
      rx_data_o  => ps2_rx_data_s,
      rx_valid_o => ps2_rx_valid_s
    );

  -----------------------------------------------------------------------------
  -- interrupt (FIRQ1; level until FIFO empty or IRQ disabled)
  -----------------------------------------------------------------------------
  irq_o <= '1' when ((fifo_n_s > 0) and (ps2_irq_s = '1')) else '0';

  -----------------------------------------------------------------------------
  -- conduit outputs -> basys3_top pins
  -----------------------------------------------------------------------------
  cfs_out_o(6 downto 0)  <= cat_s;
  cfs_out_o(7)           <= dp_s;
  cfs_out_o(11 downto 8) <= an_s;
  cfs_out_o(15 downto 12)<= vga_r_s;
  cfs_out_o(19 downto 16)<= vga_g_s;
  cfs_out_o(23 downto 20)<= vga_b_s;
  cfs_out_o(24)          <= hsync_s;
  cfs_out_o(25)          <= vsync_s;
  cfs_out_o(255 downto 26) <= (others => '0');

end architecture;