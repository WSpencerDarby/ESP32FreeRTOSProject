#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* Here is a good place to include header files that are required across
   your application. */
#include "Adafruit_MPU6050.h"

#define configUSE_PREEMPTION  1
#define [configUSE_PORT_OPTIMISED_TASK_SELECTION](#configuse_port_optimised_task_selection)                     0
#define [configUSE_TICKLESS_IDLE](#configuse_tickless_idle)                                     0
#define [configCPU_CLOCK_HZ](#configcpu_clock_hz)                                          60000000
#define [configSYSTICK_CLOCK_HZ](#configsystick_clock_hz)                                      1000000
#define configTICK_RATE_HZ 250
#define [configMAX_PRIORITIES](#configmax_priorities)                                        5
#define [configMINIMAL_STACK_SIZE](#configminimal_stack_size)                                    128
#define [configMAX_TASK_NAME_LEN](#configmax_task_name_len)                                     16
#define [configUSE_16_BIT_TICKS](#configuse_16_bit_ticks)                                      0
#define [configIDLE_SHOULD_YIELD](#configidle_should_yield)                                     1
#define configUSE_TASK_NOTIFICATIONS  1
#define [configTASK_NOTIFICATION_ARRAY_ENTRIES](#configtask_notification_array_entries)                       3
#define [configUSE_MUTEXES](#configuse_mutexes)                                           0
#define [configUSE_RECURSIVE_MUTEXES](#configuse_recursive_mutexes)                                 0
#define [configUSE_COUNTING_SEMAPHORES](#configuse_counting_semaphores)                               0
#define [configUSE_ALTERNATIVE_API](#configuse_alternative_api)                                   0 /* Deprecated! */
#define [configQUEUE_REGISTRY_SIZE](#configqueue_registry_size)                                   10
#define [configUSE_QUEUE_SETS](#configuse_queue_sets)                                        0
#define [configUSE_TIME_SLICING](#configuse_time_slicing)                                      0
#define [configUSE_NEWLIB_REENTRANT](#configuse_newlib_reentrant)                                  0
#define [configENABLE_BACKWARD_COMPATIBILITY](#configenable_backward_compatibility)                         0
#define [configNUM_THREAD_LOCAL_STORAGE_POINTERS](#confignum_thread_local_storage_pointers)                     5
#define [configUSE_MINI_LIST_ITEM](#configuse_mini_list_item)                                    1
#define [configSTACK_DEPTH_TYPE](#configstack_depth_type)                                      uint16_t
#define [configMESSAGE_BUFFER_LENGTH_TYPE](#configmessage_buffer_length_type)                            size_t
#define [configHEAP_CLEAR_MEMORY_ON_FREE](#configheap_clear_memory_on_free)                             1

/* Memory allocation related definitions. */
#define [configSUPPORT_STATIC_ALLOCATION](#configsupport_static_allocation)                             1
#define [configSUPPORT_DYNAMIC_ALLOCATION](#configsupport_dynamic_allocation)                            1
#define [configTOTAL_HEAP_SIZE](#configtotal_heap_size)                                       10240
#define [configAPPLICATION_ALLOCATED_HEAP](#configapplication_allocated_heap)                            1
#define [configSTACK_ALLOCATION_FROM_SEPARATE_HEAP](#configstack_allocation_from_separate_heap)                   1

/* Hook function related definitions. */
#define [configUSE_IDLE_HOOK](#configuse_idle_hook)                                 0
#define [configUSE_TICK_HOOK](#configuse_tick_hook)                                 0
#define [configCHECK_FOR_STACK_OVERFLOW](#configcheck_for_stack_overflow)                      0
#define [configUSE_MALLOC_FAILED_HOOK](#configuse_malloc_failed_hook)                        0
#define [configUSE_DAEMON_TASK_STARTUP_HOOK](#configuse_daemon_task_startup_hook)                  0
#define [configUSE_SB_COMPLETED_CALLBACK](#configuse_sb_completed_callback)                     0

/* Run time and task stats gathering related definitions. */
#define [configGENERATE_RUN_TIME_STATS](#configgenerate_run_time_stats)                       0
#define [configUSE_TRACE_FACILITY](#configuse_trace_facility)                            0
#define [configUSE_STATS_FORMATTING_FUNCTIONS](#configuse_stats_formatting_functions)                0

/* Co-routine related definitions. */
#define [configUSE_CO_ROUTINES](#configuse_co_routines)                               0
#define [configMAX_CO_ROUTINE_PRIORITIES](#configmax_co_routine_priorities)                     1

/* Software timer related definitions. */
#define [configUSE_TIMERS](#configuse_timers)                                    1
#define [configTIMER_TASK_PRIORITY](#configtimer_task_priority)                           3
#define [configTIMER_QUEUE_LENGTH](#configtimer_queue_length)                            10
#define [configTIMER_TASK_STACK_DEPTH](#configtimer_task_stack_depth)                        configMINIMAL_STACK_SIZE

/* Interrupt nesting behaviour configuration. */
#define [configKERNEL_INTERRUPT_PRIORITY](#kernel_priority)         [dependent of processor]
#define [configMAX_SYSCALL_INTERRUPT_PRIORITY](#kernel_priority)    [dependent on processor and application]
#define [configMAX_API_CALL_INTERRUPT_PRIORITY](#kernel_priority)   [dependent on processor and application]

/* Define to trap errors during development. */
#define [configASSERT](#configassert)( ( x ) ) if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ )

/* FreeRTOS MPU specific definitions. */
#define [configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS](#configinclude_application_defined_privileged_functions) 0
#define [configTOTAL_MPU_REGIONS](#configtotal_mpu_regions)                                8 /* Default value */
#define [configTEX_S_C_B_FLASH](#configtex_s_c_b_flash)                                  0x07UL /* Default value */
#define [configTEX_S_C_B_SRAM](#configtex_s_c_b_sram)                                   0x07UL /* Default value */
#define [configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY](#configenforce_system_calls_from_kernel_only)            1
#define [configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS](#configallow_unprivileged_critical_sections)             1
#define [configENABLE_ERRATA_837070_WORKAROUND](#configenable_errata_837070_workaround)                  1

/* ARMv8-M port specific configuration definitions. */
#define [configENABLE_TRUSTZONE](#configenable_trustzone)                            1
#define [configRUN_FREERTOS_SECURE_ONLY](#configrun_freertos_secure_only)            1
#define [configENABLE_MPU](#configenable_mpu)                                        1
#define [configENABLE_FPU](#configenable_fpu)                                        1
#define [configENABLE_MVE](#configenable_mve)                                        1

/* ARMv8-M secure side port related definitions. */
#define [secureconfigMAX_SECURE_CONTEXTS](#secureconfigMAX_SECURE_CONTEXTS)         5

/* Optional functions - most linkers will remove unused functions anyway. */
#define [INCLUDE_vTaskPrioritySet](#include_parameters)                1
#define [INCLUDE_uxTaskPriorityGet](#include_parameters)               1
#define [INCLUDE_vTaskDelete](#include_parameters)                     1
#define [INCLUDE_vTaskSuspend](#include_parameters)                    1
#define [INCLUDE_vTaskDelayUntil](#include_parameters)                 1
#define [INCLUDE_vTaskDelay](#include_parameters)                      1
#define [INCLUDE_xTaskGetSchedulerState](#include_parameters)          1
#define [INCLUDE_xTaskGetCurrentTaskHandle](#include_parameters)       1
#define [INCLUDE_uxTaskGetStackHighWaterMark](#include_parameters)     0
#define [INCLUDE_uxTaskGetStackHighWaterMark2](#include_parameters)    0
#define [INCLUDE_xTaskGetIdleTaskHandle](#include_parameters)          0
#define [INCLUDE_eTaskGetState](#include_parameters)                   0
#define [INCLUDE_xTimerPendFunctionCall](#include_parameters)          0
#define [INCLUDE_xTaskAbortDelay](#include_parameters)                 0
#define [INCLUDE_xTaskGetHandle](#include_parameters)                  0
#define [INCLUDE_xTaskResumeFromISR](#include_parameters)              1

/* A header file that defines trace macro can be included here. */

#endif /* FREERTOS_CONFIG_H */

