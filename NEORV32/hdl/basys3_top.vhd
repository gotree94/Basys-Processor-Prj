-- ================================================================================
-- basys3_top.vhd - Basys3 / NEORV32 top-level wrapper
-- -------------------------------------------------------------------------------
-- 4-kerne RISC-V soft-core "NEORV32" on Digilent Basys3 (Artix-7 XC7A35T).
--
--  * clk        : 100 MHz board oscillator (W5)
--  * boot       : BOOT_MODE_SELECT=1 -> CPU starts at 0x00000000 (IMEM)
--  * UART0      : FTDI USB-UART (19200 baud default)
--  * GPIO       : SW[15:0], BTN0..BTN3 (pin 16..19), LED[15:0]
--  * SPI0       : Pmod JA[3:0]   (sck=JA0, mosi=JA1, miso=JA2, cs=JA3)
--  * TWI        : Pmod JC[1:0]   (sda=JC0, scl=JC1)  -- open-drain
--  * PWM        : Pmod JA4       (channel 0, 서보/스피커 모듈)
--  * CFS(사용자 정의) : 7-seg / VGA / PS/2  (hdl/cfs_periph.vhd 참조)
--    - cfs_out[6:0]=seg, [7]=dp, [11:8]=an, [15:12]=R, [19:16]=G,
--      [23:20]=B, [24]=Hsync, [25]=Vsync
--    - cfs_in[0]=PS2Clk, cfs_in[1]=PS2Data
--
-- 주의: 본 파일은 "main"(2026) 브랜치 neorv32_top 포트/제네릭 기준.
--       사용하는 SW 라이브러리 경로(sw/lib)와 링커 defsym 도 같은 정책을 따름.
-- ================================================================================

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library neorv32;
use neorv32.neorv32_package.all;

entity basys3_top is
  port (
    -- 100 MHz board clock
    clk        : in  std_logic;
    -- buttons (btnC = CPU reset, active high)
    btnC       : in  std_logic;
    btnU       : in  std_logic;  -- BTN0
    btnD       : in  std_logic;  -- BTN1
    btnL       : in  std_logic;  -- BTN2
    btnR       : in  std_logic;  -- BTN3
    -- switches / leds
    sw         : in  std_logic_vector(15 downto 0);
    led        : out std_logic_vector(15 downto 0);
    -- 7-segment (active low, common anode)
    seg        : out std_logic_vector(6 downto 0); -- a..g
    dp         : out std_logic;
    an         : out std_logic_vector(3 downto 0);
    -- VGA connector
    vgaRed     : out std_logic_vector(3 downto 0);
    vgaGreen   : out std_logic_vector(3 downto 0);
    vgaBlue    : out std_logic_vector(3 downto 0);
    Hsync      : out std_logic;
    Vsync      : out std_logic;
    -- USB-UART (FTDI)
    RsTx       : out std_logic;
    RsRx       : in  std_logic;
    -- USB-HID (PS/2, inputs only in this demo)
    PS2Clk     : in  std_logic;
    PS2Data    : in  std_logic;
    -- Pmod JA: SPI(0..3) + PWM(4)
    JA         : inout std_logic_vector(7 downto 0);
    -- Pmod JC: I2C/TWI(0..1)
    JC         : inout std_logic_vector(7 downto 0)
  );
end entity;

architecture structural of basys3_top is

  -- reset
  signal rstn_c : std_ulogic;

  -- GPIO
  signal gpio_dir_s : std_ulogic_vector(31 downto 0);
  signal gpio_o_s   : std_ulogic_vector(31 downto 0);
  signal gpio_i_s   : std_ulogic_vector(31 downto 0);

  -- UART0
  signal uart0_txd_s : std_ulogic;

  -- SPI
  signal spi_csn_s : std_ulogic_vector(7 downto 0);
  signal spi_sck_s : std_ulogic;
  signal spi_mosi_s: std_ulogic;
  signal spi_miso_s: std_ulogic;

  -- TWI (open-drain)
  signal twi_sda_o_s, twi_scl_o_s : std_ulogic;
  signal twi_sda_i_s, twi_scl_i_s : std_ulogic;

  -- PWM
  signal pwm_s : std_ulogic_vector(31 downto 0);

  -- CFS conduits
  signal cfs_in_s  : std_ulogic_vector(255 downto 0);
  signal cfs_out_s : std_ulogic_vector(255 downto 0);

begin

  -- reset (btnC pressed -> CPU reset)
  rstn_c <= '0' when (btnC = '1') else '1';

  -- ------------------------------------------------------------------------------
  -- GPIO wiring (24 pins)
  --   [15:0]  = SW (input, dir=0)
  --   [19:16] = BTN0..BTN3 (input, dir=0)
  --   LED     = gpio_o[15:0] (dir=1; SW code sets 0x0000FFFF)
  -- ------------------------------------------------------------------------------
  gpio_i_s(15 downto 0)    <= std_ulogic_vector(sw);
  gpio_i_s(16)             <= std_ulogic(btnU);  -- BTN0
  gpio_i_s(17)             <= std_ulogic(btnD);  -- BTN1
  gpio_i_s(18)             <= std_ulogic(btnL);  -- BTN2
  gpio_i_s(19)             <= std_ulogic(btnR);  -- BTN3
  gpio_i_s(31 downto 20)   <= (others => '0');

  led <= std_logic_vector(gpio_o_s(15 downto 0) and gpio_dir_s(15 downto 0));

  -- ------------------------------------------------------------------------------
-- UART0 -> FTDI
  ------------------------------------------------------------------------------
  RsTx        <= std_logic(uart0_txd_s);

  -- ------------------------------------------------------------------------------
  -- SPI0 -> Pmod JA
  -- ------------------------------------------------------------------------------
  JA(0) <= std_logic(spi_sck_s);                  -- SCK
  JA(1) <= std_logic(spi_mosi_s);                 -- MOSI
  JA(3) <= std_logic(spi_csn_s(0));               -- CS (active low)
  JA(2) <= 'Z';                                   -- MISO (input)
  spi_miso_s <= std_ulogic(JA(2));
  JA(4) <= std_logic(pwm_s(0));                   -- PWM ch0 outlet
  JA(7 downto 5) <= (others => '0');

  -- ------------------------------------------------------------------------------
  -- TWI -> Pmod JC (open-drain, pull low only)
  -- ------------------------------------------------------------------------------
  JC(0) <= '0' when (twi_sda_o_s = '0') else 'Z';
  JC(1) <= '0' when (twi_scl_o_s = '0') else 'Z';
  twi_sda_i_s <= std_ulogic(JC(0));
  twi_scl_i_s <= std_ulogic(JC(1));
  JC(7 downto 2) <= (others => 'Z');

  -- ------------------------------------------------------------------------------
  -- CFS conduits -> on-board peripherals
  -- ------------------------------------------------------------------------------
  seg          <= std_logic_vector(cfs_out_s(6 downto 0));
  dp           <= std_logic(cfs_out_s(7));
  an           <= std_logic_vector(cfs_out_s(11 downto 8));
  vgaRed       <= std_logic_vector(cfs_out_s(15 downto 12));
  vgaGreen     <= std_logic_vector(cfs_out_s(19 downto 16));
  vgaBlue      <= std_logic_vector(cfs_out_s(23 downto 20));
  Hsync        <= std_logic(cfs_out_s(24));
  Vsync        <= std_logic(cfs_out_s(25));

  cfs_in_s(0)  <= std_ulogic(PS2Clk);
  cfs_in_s(1)  <= std_ulogic(PS2Data);
  cfs_in_s(255 downto 2) <= (others => '0');

  -- ------------------------------------------------------------------------------
  -- NEORV32 SoC
  -- ------------------------------------------------------------------------------
  neorv32_top_inst : entity neorv32.neorv32_top
    generic map (
      -- general
      CLOCK_FREQUENCY     => 100000000,
      BOOT_MODE_SELECT    => 1,
      BOOT_ADDR_CUSTOM    => x"00000000",
      -- ISA
      RISCV_ISA_C         => true,     -- match -march=rv32imc
      RISCV_ISA_M         => true,
      RISCV_ISA_Zicntr    => true,
      -- memory (firmware linked via --defsym=__neorv32_rom_size/ram_size)
      IMEM_EN             => true,
      IMEM_SIZE           => 32*1024,
      DMEM_EN             => true,
      DMEM_SIZE           => 16*1024,
      -- peripherals
      IO_GPIO_NUM         => 24,
      IO_GPIO_DIR_EN      => true,
      IO_CLINT_EN         => true,
      IO_UART0_EN         => true,
      IO_UART0_RX_FIFO    => 1,
      IO_UART0_TX_FIFO    => 1,
      IO_SPI_EN           => true,
      IO_SPI_FIFO         => 1,
      IO_TWI_EN           => true,
      IO_TWI_FIFO         => 1,
      IO_PWM_NUM          => 4,
      IO_DMA_EN           => true,
      IO_DMA_DSC_FIFO     => 4,
      IO_CFS_EN           => true
      -- 모든 나머지 제네릭은 neorv32_top.vhd 의 기본값 사용
    )
    port map (
      -- global
      clk_i   => std_ulogic(clk),
      rstn_i  => rstn_c,
      -- unused outputs (disabled peripherals)
      rstn_ocd_o => open,
      rstn_wdt_o => open,
      trace_cpu0_o => open,
      trace_cpu1_o => open,
      jtag_tdo_o  => open,
      smc_ioen_o  => open,
      smc_sck_o   => open,
      smc_csn_o   => open,
      smc_sdo_o   => open,
      xbus_adr_o  => open,
      xbus_dat_o  => open,
      xbus_cti_o  => open,
      xbus_tag_o  => open,
      xbus_we_o   => open,
      xbus_sel_o  => open,
      xbus_stb_o  => open,
      xbus_cyc_o  => open,
      slink_rx_rdy_o => open,
      slink_tx_dat_o => open,
      slink_tx_dst_o => open,
      slink_tx_val_o => open,
      slink_tx_lst_o => open,
      uart0_rtsn_o => open,
      uart1_txd_o  => open,
      uart1_rtsn_o => open,
      sdi_dat_o    => open,
      twd_sda_o    => open,
      onewire_o    => open,
      neoled_o     => open,
      mtime_time_o => open,
      -- GPIO
      gpio_dir_o => gpio_dir_s,
      gpio_o     => gpio_o_s,
      gpio_i     => gpio_i_s,
      -- UART0
      uart0_txd_o => uart0_txd_s,
      uart0_rxd_i => std_ulogic(RsRx),
      -- SPI
      spi_clk_o => spi_sck_s,
      spi_dat_o => spi_mosi_s,
      spi_dat_i => spi_miso_s,
      spi_csn_o => spi_csn_s,
      -- TWI
      twi_sda_o => twi_sda_o_s,
      twi_sda_i => twi_sda_i_s,
      twi_scl_o => twi_scl_o_s,
      twi_scl_i => twi_scl_i_s,
      -- PWM
      pwm_o => pwm_s,
      -- CFS
      cfs_in_i  => cfs_in_s,
      cfs_out_o => cfs_out_s,
      -- CPU interrupts (not used externally)
      irq_msi_i => '0',
      irq_mti_i => '0',
      irq_mei_i => '0'
    );

end architecture;