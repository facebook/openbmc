/*
 * plat_vr_init:
 * 	This function will be called by vr_probe()
 * 	for platform to register the voltage regulators
 */
int plat_vr_init(void)
{
  return -1;
}

/*
 * plat_vr_exit:
 * 	This function will be called by vr_remove()
 * 	before unreigstering all of the voltage regulators
 */
void plat_vr_exit(void)
{
  return;
}
