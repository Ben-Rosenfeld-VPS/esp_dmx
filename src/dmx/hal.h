#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "soc/uart_struct.h"

#define UART_LL_TOUT_REF_FACTOR_DEFAULT (8)  // The timeout calibration factor when using ref_tick

#define REG_UART_BASE(i)      (DR_REG_UART_BASE + (i) * 0x10000 + ((i) > 1 ? 0xe000 : 0 ))
#define REG_UART_AHB_BASE(i)  (0x60000000 + (i) * 0x10000 + ((i) > 1 ? 0xe000 : 0 ))
#define UART_FIFO_AHB_REG(i)  (REG_UART_AHB_BASE(i) + 0x0)
#define UART_FIFO_REG(i)      (REG_UART_BASE(i) + 0x0)

/**
 * @brief The the interrupt status mask from the UART.
 * 
 * @param dev Pointer to a UART struct.
 * 
 * @return The interrupt status mask. 
 */
static inline uint32_t dmx_hal_get_intsts_mask(uart_dev_t *dev) {
  return dev->int_st.val;
}

/**
 * @brief Enables UART interrupts using an interrupt mask.
 * 
 * @param dev Pointer to a UART struct.
 * @param mask The UART mask that is enabled.
 */
static inline void dmx_hal_ena_intr_mask(uart_dev_t *dev, uint32_t mask) {
  dev->int_ena.val |= mask;
}

/**
 * @brief Disables UART interrupts using an interrupt mask.
 * 
 * @param dev Pointer to a UART struct.
 * @param mask The UART mask that is disabled.
 */
static inline void dmx_hal_disable_intr_mask(uart_dev_t *dev, uint32_t mask) {
  dev->int_ena.val &= (~mask);
}

/**
 * @brief Clears UART interrupts using a mask.
 * 
 * @param dev Pointer to a UART struct.
 * @param mask The UART mask that is cleared.
 */
static inline void dmx_hal_clr_intsts_mask(uart_dev_t *dev, uint32_t mask) {
  dev->int_clr.val = mask;
}

/**
 * @brief Get the current length of the bytes in the rx FIFO.
 * 
 * @param hal Context of the HAL layer
 * 
 * @return Number of bytes in the rx FIFO
 */
static inline IRAM_ATTR uint32_t dmx_hal_get_rxfifo_len(uart_dev_t *dev) {
  uint32_t fifo_cnt = dev->status.rxfifo_cnt;
  typeof(dev->mem_rx_status) rx_status = dev->mem_rx_status;
  uint32_t len = 0;

  // When using DPort to read fifo, fifo_cnt is not credible, we need to calculate the real cnt based on the fifo read and write pointer.
  // When using AHB to read FIFO, we can use fifo_cnt to indicate the data length in fifo.
  if (rx_status.wr_addr > rx_status.rd_addr) {
    len = rx_status.wr_addr - rx_status.rd_addr;
  } else if (rx_status.wr_addr < rx_status.rd_addr) {
    len = (rx_status.wr_addr + 128) - rx_status.rd_addr;
  } else {
    len = fifo_cnt > 0 ? 128 : 0;
  }

  return len;
}

/**
 * @brief Gets the number of bits the UART remains idle after transmitting data.
 * 
 * @param dev Pointer to a UART struct.
 * @return The number of bits the UART is idle after transmitting data. 
 */
static inline uint16_t dmx_hal_get_idle_num(uart_dev_t *dev) {
  return dev->idle_conf.tx_idle_num;
}

/**
 * @brief Gets the number of bits the UART sends as break.
 * 
 * @param dev Pointer to a UART struct.
 * @return The number of bits the UART sends as a break after transmitting.
 */
static inline uint8_t dmx_hal_get_break_num(uart_dev_t *dev) {
  return dev->idle_conf.tx_brk_num;
}

/**
 * @brief Gets the UART rx timeout (unit: time it takes for one word to be sent at current baud_rate).
 * 
 * @param dev Pointer to a UART struct.
 * @return The UART rx timeout.
 */
static inline uint8_t dmx_hal_get_rx_tout(uart_dev_t *dev) {
  return dev->conf1.rx_tout_en ? dev->conf1.rx_tout_thrhd : 0;
}

/**
 * @brief Inverts or uninverts tx line on the UART bus.
 * 
 * @param dev Pointer to a UART struct.
 * @param invert 1 to invert, 0 to un-invert.
 */
static inline void dmx_hal_inverse_txd_signal(uart_dev_t *dev, int invert) {
  dev->conf0.txd_inv = invert ? 1 : 0;
}

/**
 * @brief Inverts or uninverts rts line on the UART bus.
 * 
 * @param dev Pointer to a UART struct.
 * @param invert 1 to invert, 0 to un-invert.
 */
static inline void dmx_hal_inverse_rts_signal(uart_dev_t *dev, int invert) {
  dev->conf0.rts_inv = invert ? 1 : 0;
}

/**
 * @brief Gets the level of the rx line on the UART bus.
 * 
 * @param dev Pointer to a UART struct.
 * @return UART rx line level.
 */
static inline uint32_t dmx_hal_get_rx_level(uart_dev_t *dev) {
  return dev->status.rxd;
}

/**
 * @brief Read the first num characters from the rxfifo.
 * 
 * @param dev Pointer to a UART struct.
 * @param buf Destination buffer to be read into
 * @param num The maximum number of characters to read
 * 
 * @return The number of characters read
 */
static inline IRAM_ATTR int dmx_hal_readn_rxfifo(uart_dev_t *dev, uint8_t *buf, int num) {
  const uint16_t rxfifo_len = dmx_hal_get_rxfifo_len(dev);
  if (num > rxfifo_len) num = rxfifo_len;
  
  // read the rxfifo
  for(int i = 0; i < num; i++) {
    buf[i] = dev->fifo.rw_byte;
#ifdef CONFIG_COMPILER_OPTIMIZATION_PERF
    // perform a nop if compiled to optimize performance
    // not sure why this is here, but we'll keep it for now
    __asm__ __volatile__("nop");
#endif
  }

  return num;
}

/**
 * @brief Enables or disables the UART RTS line.
 * 
 * @param dev Pointer to a UART struct.
 * @param set 1 to enable the RTS line (set low), 0 to disable the RTS line (set high).
 */
static inline void dmx_hal_set_rts(uart_dev_t *dev, int set) {
  dev->conf0.sw_rts = set & 0x1;
}

/**
 * @brief Gets the enabled UART interrupt status.
 * 
 * @param dev Pointer to a UART struct.
 * @return Gets the enabled UART interrupt status.
 */
static inline uint32_t dmx_hal_get_intr_ena_status(uart_dev_t *dev){
  return dev->int_ena.val;
}

/**
 * @brief Initializes the UART for DMX mode.
 * 
 * @param dev Pointer to a UART struct.
 * @param dmx_num The UART number to initialize.
 */
static inline void dmx_hal_init(uart_dev_t *dev, dmx_port_t dmx_num) {
  // disable parity
  dev->conf0.parity_en = 0x0;

  // set 8 data bits
  dev->conf0.bit_num = 0x3; 

  // set 2 stop bits - enable rs485 mode as hardware workaround
  dev->rs485_conf.dl1_en = 0x1;
  dev->conf0.stop_bit_num = 0x1;

  // disable flow control
  dev->conf1.rx_flow_en = 0x0;
  dev->conf0.tx_flow_en = 0x0;

  // enable rs485 collision detection
  dev->conf0.irda_en = 0x0;
  dev->rs485_conf.tx_rx_en = 0x1; // output loop back to the receivers input
  dev->rs485_conf.rx_busy_tx_en = 0x1; // send data when the receiver is busy
  dev->conf0.sw_rts = 0x0;
  dev->rs485_conf.en = 0x1;
}

/**
 * @brief Sets the baud rate for the UART.
 * 
 * @param dev Pointer to a UART struct.
 * @param source_clk The source of the UART hardware clock to use. 
 * @param baud_rate The baud rate to use.
 */
static inline void dmx_hal_set_baudrate(uart_dev_t *dev, uart_sclk_t source_clk, int baud_rate) {
  uint32_t sclk_freq = (source_clk == UART_SCLK_APB) ? APB_CLK_FREQ : REF_CLK_FREQ;
  uint32_t clk_div = ((sclk_freq) << 4) / baud_rate;
  
  // baud-rate configuration register is divided into an integer part and a fractional part
  dev->clk_div.div_int = clk_div >> 4;
  dev->clk_div.div_frag = clk_div & 0xf;

  // configure the uart source clock
  dev->conf0.tick_ref_always_on = (source_clk == UART_SCLK_APB);
}

/**
 * @brief Sets the number of mark bits to transmit after a break has been transmitted.
 * 
 * @param dev Pointer to a UART struct.
 * @param idle_num The number of idle bits to transmit.
 */
static inline void dmx_hal_set_tx_idle_num(uart_dev_t *dev, uint16_t idle_num) {
  // TODO: make this like dmx_hal_set_tx_break_num() where 0 disables idle num?
  dev->idle_conf.tx_idle_num = idle_num;
}

/**
 * @brief Enables or disables transmitting UART hardware break.
 * 
 * @param dev Pointer to a UART struct.
 * @param break_num The number of break bits to transmit when a break is transmitted.
 */
static inline void dmx_hal_set_tx_break_num(uart_dev_t *dev, uint8_t break_num) {
  if (break_num > 0) {
    dev->idle_conf.tx_brk_num = break_num;
    dev->conf0.txd_brk = 1;
  } else {
    dev->conf0.txd_brk = 0;
  }
}

/**
 * @brief Get the source clock for the UART hardware. 
 * 
 * @param dev Pointer to a UART struct.
 * @param source_clk The ID of the source clock used for the UART hardware.
 */
static inline IRAM_ATTR uart_sclk_t dmx_hal_get_sclk(uart_dev_t *dev) {
  return dev->conf0.tick_ref_always_on ? UART_SCLK_APB : UART_SCLK_REF_TICK;
}

/**
 * @brief Get the UART baud rate of the selected UART hardware.
 * 
 * @param dev Pointer to a UART struct.
 * 
 * @return The baud rate of the UART hardware. 
 */
static inline IRAM_ATTR uint32_t dmx_hal_get_baudrate(uart_dev_t *dev) {
  uint32_t src_clk = dev->conf0.tick_ref_always_on ? APB_CLK_FREQ : REF_CLK_FREQ;
  typeof(dev->clk_div) div_reg = dev->clk_div;
  return (src_clk << 4) / ((div_reg.div_int << 4) | div_reg.div_frag);
}

/**
 * @brief Set the duration for the UART RX inactivity timeout that triggers the RX timeout interrupt.
 * 
 * @param dev Pointer to a UART struct.
 * @param rx_timeout_thresh The RX timeout duration (unit: time of sending one byte).
 */
static inline IRAM_ATTR void dmx_hal_set_rx_timeout(uart_dev_t *dev, uint8_t rx_timeout_thresh) {
  if (dev->conf0.tick_ref_always_on == 0) {
    // when using ref_tick, the rx timeout threshold needs increase to 10 times
    rx_timeout_thresh *= UART_LL_TOUT_REF_FACTOR_DEFAULT;
  } else {
    // if APB_CLK is used, counting rate is baud tick rate / 8
    rx_timeout_thresh = (rx_timeout_thresh + 7) / 8;
  }
  if (rx_timeout_thresh > 0) {
    dev->conf1.rx_tout_thrhd = rx_timeout_thresh;
    dev->conf1.rx_tout_en = 1;
  } else {
    dev->conf1.rx_tout_en = 0;
  }
}

/**
 * @brief Sets the number of bytes that the UART must receive to trigger a RX FIFO full interrupt.
 * 
 * @param dev Pointer to a UART struct.
 * @param rxfifo_full_thresh The number of bytes needed to trigger an RX FIFO full interrupt.
 */
static inline IRAM_ATTR void dmx_hal_set_rxfifo_full_thr(uart_dev_t *dev, uint8_t rxfifo_full_thresh) {
  dev->conf1.rxfifo_full_thrhd = rxfifo_full_thresh;
}

/**
 * @brief Sets the number of bytes that the UART TX FIFO must have remaining in it to trigger a TX FIFO empty interrupt.
 * 
 * @param dev Pointer to a UART struct.
 * @param threshold The number of bytes remaining to trigger a TX FIFO empty interrupt.
 */
static inline IRAM_ATTR void dmx_hal_set_txfifo_empty_thr(uart_dev_t *dev, uint8_t threshold) {
  dev->conf1.txfifo_empty_thrhd = threshold;
}

/**
 * @brief Resets the UART RX FIFO.
 * 
 * @param dev Pointer to a UART struct.
 */
static inline IRAM_ATTR void dmx_hal_rxfifo_rst(uart_dev_t *dev) {
  // hardware issue: we can not use `rxfifo_rst` to reset the dev rxfifo.
  uint16_t fifo_cnt;
  typeof(dev->mem_rx_status) rxmem_sta;
  // get the UART APB fifo addr
  uint32_t fifo_addr = (dev == &UART0) ? UART_FIFO_REG(0) : (dev == &UART1) 
    ? UART_FIFO_REG(1) : UART_FIFO_REG(2);
  do {
    fifo_cnt = dev->status.rxfifo_cnt;
    rxmem_sta.val = dev->mem_rx_status.val;
    if(fifo_cnt != 0 ||  (rxmem_sta.rd_addr != rxmem_sta.wr_addr)) {
      READ_PERI_REG(fifo_addr);
    } else {
      break;
    }
  } while (1);
}

/**
 * @brief Get the length of the UART TX FIFO.
 * 
 * @param dev Pointer to a UART struct.
 * @return The length of the UART TX FIFO. 
 */
static inline IRAM_ATTR uint32_t dmx_hal_get_txfifo_len(uart_dev_t *dev) {
  // default fifo len - fifo count
  return 128 - dev->status.txfifo_cnt;
}

static inline IRAM_ATTR void dmx_hal_write_txfifo(uart_dev_t *dev, const uint8_t *buf, uint32_t data_size, uint32_t *write_size) {
  uint16_t wr_len = dmx_hal_get_txfifo_len(dev);
  if (wr_len > data_size) wr_len = data_size;
  *write_size = wr_len;
  
  // write to the txfifo using AHB address
  uint32_t fifo_addr = (dev == &UART0) ? UART_FIFO_AHB_REG(0) : (dev == &UART1)
    ? UART_FIFO_AHB_REG(1) : UART_FIFO_AHB_REG(2);
  for (int i = 0; i < wr_len; i++) WRITE_PERI_REG(fifo_addr, buf[i]);
}

static inline IRAM_ATTR void dmx_hal_txfifo_rst(uart_dev_t *dev) {
  /*
   * Note:   Due to hardware issue, reset UART1's txfifo will also reset UART2's txfifo.
   *         So reserve this function for UART1 and UART2. Please do DPORT reset for UART and its memory at chip startup
   *         to ensure the TX FIFO is reset correctly at the beginning.
   */
  dev->conf0.txfifo_rst = 1;
  dev->conf0.txfifo_rst = 0;
}

#ifdef __cplusplus
}
#endif
