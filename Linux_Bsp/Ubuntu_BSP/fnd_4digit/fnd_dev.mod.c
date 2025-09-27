#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xc40716a6, "kthread_stop" },
	{ 0xcc4bc881, "gpio_to_desc" },
	{ 0x326eca89, "gpiod_set_raw_value" },
	{ 0xfe990052, "gpio_free" },
	{ 0x2d222da1, "misc_deregister" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xae353d77, "arm_copy_from_user" },
	{ 0xc358aaf8, "snprintf" },
	{ 0x5f754e5a, "memset" },
	{ 0x7682ba4e, "__copy_overflow" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xffbaf89d, "gpiod_direction_output_raw" },
	{ 0xe6fa52e0, "kthread_create_on_node" },
	{ 0x187d7c48, "wake_up_process" },
	{ 0xa2928db5, "misc_register" },
	{ 0x92997ed8, "_printk" },
	{ 0x8e865d3c, "arm_delay_ops" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0x8a3dbe73, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "480825C71DF040117647EDE");
