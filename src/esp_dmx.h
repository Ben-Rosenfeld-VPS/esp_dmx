/**
 * @file esp_dmx.h
 * @author Mitch Weisbrod
 * @brief This is the main header file for esp_dmx. This file declares functions
 * needed for installing the DMX driver and sending or receiving DMX data. It is
 * possible to implement RDM using the functions found in this header file
 * alone. However, RDM can be complex to users who aren't familiar with the
 * standard. Functions found in rdm/agent.h and rdm/requests.h can be used to
 * simplify basic RDM tasks.
 */
#pragma once

#include "dmx/agent.h"
#include "dmx/types.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reads DMX data from the driver into a destination buffer with an
 * offset. This can be useful when a receiving DMX device only needs to process
 * a small footprint of the DMX packet.
 *
 * @param dmx_num The DMX port number.
 * @param offset The number of slots with which to offset the read. If set to 0
 * this function is equivalent to dmx_read().
 * @param[out] destination The destination buffer into which to read the DMX
 * data.
 * @param size The size of the destination buffer.
 * @return The number of bytes read from the DMX driver.
 */
size_t dmx_read_offset(dmx_port_t dmx_num, size_t offset, void *destination,
                       size_t size);

/**
 * @brief Reads DMX data from the driver into a destination buffer.
 *
 * @param dmx_num The DMX port number.
 * @param[out] destination The destination buffer into which to read the DMX
 * data.
 * @param size The size of the destination buffer.
 * @return The number of bytes read from the DMX driver.
 */
size_t dmx_read(dmx_port_t dmx_num, void *destination, size_t size);

/**
 * @brief Reads a single slot of DMX data.
 *
 * @param dmx_num The DMX port number.
 * @param slot_num The DMX slot number to read.
 * @return The value of the DMX slot or -1 on error.
 */
int dmx_read_slot(dmx_port_t dmx_num, size_t slot_num);

/**
 * @brief Writes DMX data from a source buffer into the DMX driver buffer with
 * an offset. Allows a source buffer to be written to a specific slot number in
 * the DMX driver buffer.
 *
 * @param dmx_num The DMX port number.
 * @param offset The number of slots with which to offset the write. If set to 0
 * this function is equivalent to dmx_write().
 * @param[in] source The source buffer which is copied to the DMX driver.
 * @param size The size of the source buffer.
 * @return The number of bytes written into the DMX driver.
 */
size_t dmx_write_offset(dmx_port_t dmx_num, size_t offset, const void *source,
                        size_t size);

/**
 * @brief Writes DMX data from a source buffer into the DMX driver buffer. Data
 * written into the DMX driver buffer can then be sent to DMX devices.
 *
 * @param dmx_num The DMX port number.
 * @param[in] source The source buffer which is copied to the DMX driver.
 * @param size The size of the source buffer.
 * @return The number of bytes written into the DMX driver.
 */
size_t dmx_write(dmx_port_t dmx_num, const void *source, size_t size);

/**
 * @brief Writes a single slot of DMX data.
 *
 * @param dmx_num The DMX port number.
 * @param slot_num The DMX slot number to write.
 * @return The value written to the DMX slot or -1 on error.
 */
int dmx_write_slot(dmx_port_t dmx_num, size_t slot_num, uint8_t value);

/**
 * @brief Receives a DMX packet from the DMX bus. This is a blocking function.
 * This function first blocks until the DMX driver is idle and then it blocks
 * using a timeout until a new packet is received. This function will timeout
 * early according to RDM specification if an RDM packet is expected.
 *
 * @note This function uses FreeRTOS direct-to-task notifications to block and
 * unblock. Using task notifications on the same task that calls this function
 * can lead to undesired behavior and program instability.
 *
 * @param dmx_num The DMX port number.
 * @param[out] packet An optional pointer to a dmx_packet_t which contains
 * information about the received DMX packet.
 * @param wait_ticks The number of ticks to wait before this function times out.
 * @return The size of the received DMX packet or 0 if no packet was received.
 */
size_t dmx_receive(dmx_port_t dmx_num, dmx_packet_t *packet,
                   TickType_t wait_ticks);

/**
 * @brief Sends a DMX packet on the DMX bus. This function blocks indefinitely
 * until the DMX driver is idle and then sends a packet.
 *
 * @note This function uses FreeRTOS direct-to-task notifications to block and
 * unblock. Using task notifications on the same task that calls this function
 * can lead to undesired behavior and program instability.
 *
 * @param dmx_num The DMX port number.
 * @param size The size of the packet to send. If 0, sends the number of bytes
 * equal to the highest slot number that was written or sent in the previous
 * call to dmx_write(), dmx_write_offset(), dmx_write_slot(), or dmx_send().
 * @return The number of bytes sent on the DMX bus.
 */
size_t dmx_send(dmx_port_t dmx_num, size_t size);

/**
 * @brief Waits until the DMX packet is done being sent. This function can be
 * used to ensure that calls to dmx_write() happen synchronously with the
 * current DMX frame.
 *
 * @note This function uses FreeRTOS direct-to-task notifications to block and
 * unblock. Using task notifications on the same task that calls this function
 * can lead to undesired behavior and program instability.
 *
 * @param dmx_num The DMX port number.
 * @param wait_ticks The number of ticks to wait before this function times out.
 * @retval true if the DMX driver is done sending.
 * @retval false if the function timed out.
 */
bool dmx_wait_sent(dmx_port_t dmx_num, TickType_t wait_ticks);

#ifdef __cplusplus
}
#endif
