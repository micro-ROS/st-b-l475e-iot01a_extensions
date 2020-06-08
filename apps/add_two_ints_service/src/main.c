#include <zephyr.h>
#include <device.h>
#include <stdio.h>
#include <sys/util.h>
#include <string.h>
#include <sys/printk.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <std_msgs/msg/int32.h>
#include "example_interfaces/srv/add_two_ints.h"

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printk("Failed status on line %d: %d. Aborting.\n",__LINE__,(int)temp_rc);}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printk("Failed status on line %d: %d. Continuing.\n",__LINE__,(int)temp_rc);}}

void main(void)
{	
	rcl_init_options_t options = rcl_get_zero_initialized_init_options();
	RCCHECK(rcl_init_options_init(&options, rcl_get_default_allocator()))

	// Optional RMW configuration 
	rmw_init_options_t* rmw_options = rcl_init_options_get_rmw_init_options(&options);
	rmw_uros_options_set_serial_device("1", rmw_options);
	RCCHECK(rmw_uros_options_set_client_key(0xDEADBEEF, rmw_options))

	rcl_context_t context = rcl_get_zero_initialized_context();
	RCCHECK(rcl_init(0, NULL, &options, &context))

	rcl_node_options_t node_ops = rcl_node_get_default_options();
	rcl_node_t node = rcl_get_zero_initialized_node();
	RCCHECK(rcl_node_init(&node, "addtowints_server_rcl", "", &context, &node_ops))

	const char * service_name = "addtwoints";
	rcl_service_options_t service_op = rcl_service_get_default_options();
	rcl_service_t serv = rcl_get_zero_initialized_service();
	const rosidl_service_type_support_t * service_type_support = ROSIDL_GET_SRV_TYPE_SUPPORT(example_interfaces, srv, AddTwoInts);

	RCCHECK(rcl_service_init(&serv,&node, service_type_support, service_name, &service_op))

	rcl_wait_set_t wait_set = rcl_get_zero_initialized_wait_set();
	RCCHECK(rcl_wait_set_init(&wait_set, 0, 0, 0, 0, 1, 0, &context, rcl_get_default_allocator()))

	do {
		RCSOFTCHECK(rcl_wait_set_clear(&wait_set))

		size_t index;
		RCSOFTCHECK(rcl_wait_set_add_service(&wait_set, &serv, &index))

		rcl_wait(&wait_set, RCL_MS_TO_NS(100));

		if (wait_set.services[index]) {   
			rmw_request_id_t req_id;
			example_interfaces__srv__AddTwoInts_Request req;
			example_interfaces__srv__AddTwoInts_Request__init(&req);
			RCSOFTCHECK(rcl_take_request(&serv,&req_id,&req))

			example_interfaces__srv__AddTwoInts_Response res;
			example_interfaces__srv__AddTwoInts_Response__init(&res);

			res.sum = req.a + req.b;

			RCSOFTCHECK(rcl_send_response(&serv,&req_id,&res))
		}
        k_sleep(100);
	} while ( true );

  RCCHECK(rcl_service_fini(&serv,&node))
  RCCHECK(rcl_node_fini(&node))
}
