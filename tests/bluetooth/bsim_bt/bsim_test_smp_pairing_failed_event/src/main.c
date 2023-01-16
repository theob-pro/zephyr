#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_bsim_smp_pairing_failed_event, LOG_LEVEL_DBG);

#define FAIL(...)					\
	do {						\
		bst_result = Failed;			\
		bs_trace_error_time_line(__VA_ARGS__);	\
	} while (0)

#define PASS(...)					\
	do {						\
		bst_result = Passed;			\
		bs_trace_info_time(1, __VA_ARGS__);	\
	} while (0)

extern enum bst_result_t bst_result;

#define WAIT_TIME (20e6) /* 20 seconds */

extern void test_central_main();
extern void test_peripheral_main();



static void test_central() {
	test_central_main();
}

static void test_peripheral() {
	test_peripheral_main();
}

void test_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("Test failed (not passed after %i seconds)\n", WAIT_TIME);
	}
}

static void test_smp_pairing_failed_event_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME);
	bst_result = In_progress;
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central waiting for PDU",
		.test_post_init_f = test_smp_pairing_failed_event_init,
		.test_tick_f = test_tick,
		.test_main_f = test_central
	},
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral sending SMP pairing PDU",
		.test_post_init_f = test_smp_pairing_failed_event_init,
		.test_tick_f = test_tick,
		.test_main_f = test_peripheral
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_smp_pairing_failed_event_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_smp_pairing_failed_event_install,
	NULL
};

void main(void)
{
        bst_main();
}
