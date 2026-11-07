/* SPDX-License-Identifier: GPL-3.0-only
 * Compiler ordering for the FM1's BLE IRQ -> main loop SPSC queue.
 * This is ONLY a compiler memory barrier, not a CPU/core hardware
 * memory barrier. The queue must remain in coherent on-chip SRAM;
 * a future dual-core producer would need a stronger synchronization
 * contract. Keep all inline assembly in firmware/hal/.
 */
#ifndef FM1_BT_BARRIER_H
#define FM1_BT_BARRIER_H
#define SLOOP_BT_COMPILER_BARRIER() __asm__ __volatile__("" ::: "memory")
#endif
