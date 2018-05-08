#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/module.h>

#define WITH_VALE
#include <bsd_glue.h>
#include <net/netmap.h>
#include <dev/netmap/netmap_kern.h>



#define MY_BDG "vale0:"
#define MY_PORT0 "v0"
#define MY_PORT1 "v1"
#define MY_PORT2 "v2"


static uint32_t my_module_lookup(struct nm_bdg_fwd *, uint8_t *,
	struct netmap_vp_adapter *, void *);

static struct netmap_bdg_ops my_bdg_ops = {my_module_lookup, NULL, NULL};
void *auth_token = NULL;



static int
create_vale_port(const char* name)
{
	struct nmreq_vale_newif newif;
	struct nmreq_header hdr;

	bzero(&hdr, sizeof(hdr));
	hdr.nr_version = NETMAP_API;
	hdr.nr_reqtype = NETMAP_REQ_VALE_NEWIF;
	strncpy(hdr.nr_name, name, sizeof(hdr.nr_name));

	bzero(&newif, sizeof(newif));
	hdr.nr_body = (uint64_t)&newif;

	return nm_vi_create(&hdr);
}



static inline int
destroy_vale_port(const char* name)
{

	return nm_vi_destroy(name);
}



/* If the call succeed, port_index will contain the position of the port inside
 * the VALE bridge
 */
static int
attach_port(const char *bdg_name, const char *port_name, void *auth_token,
	uint32_t *port_index)
{
	struct nmreq_vale_attach nmr_att;
	struct nmreq_header hdr;
	int ret = 0;

	bzero(&nmr_att, sizeof(nmr_att));
	nmr_att.reg.nr_mode = NR_REG_ALL_NIC;

	bzero(&hdr, sizeof(hdr));
	hdr.nr_version = NETMAP_API;
	hdr.nr_reqtype = NETMAP_REQ_VALE_ATTACH;
	hdr.nr_body = (uint64_t)&nmr_att;
	snprintf(hdr.nr_name, sizeof(hdr.nr_name), "%s%s", bdg_name, port_name);

	ret = nm_bdg_ctl_attach(&hdr, auth_token);
	if (port_index != NULL) {
		*port_index = nmr_att.port_index;
	}

	return ret;
}



/* If the call succeed, port_index will contain the position the port had inside
 * the VALE bridge
 */
static int
detach_port(const char *bdg_name, const char *port_name, void *auth_token,
	uint32_t *port_index)
{
	struct nmreq_vale_detach nmr_det;
	struct nmreq_header hdr;
	int ret = 0;

	bzero(&nmr_det, sizeof(nmr_det));

	bzero(&hdr, sizeof(hdr));
	hdr.nr_version = NETMAP_API;
	hdr.nr_reqtype = NETMAP_REQ_VALE_DETACH;
	hdr.nr_body = (uint64_t)&nmr_det;
	snprintf(hdr.nr_name, sizeof(hdr.nr_name), "%s%s", bdg_name, port_name);

	ret = nm_bdg_ctl_detach(&hdr, auth_token);
	*port_index = nmr_det.port_index;

	return ret;
}



static uint32_t
my_module_lookup(struct nm_bdg_fwd *ft, uint8_t *dst_ring,
	struct netmap_vp_adapter *vpna, void *lookup_data)
{
	uint32_t bdg_port = vpna->bdg_port;

	if (bdg_port == 1 || bdg_port == 2) {
		return 0;
	}

	return NM_BDG_BROADCAST;
}



static int __init
my_module_init(void)
{
	int port_index;
	int ret;

	/* Creates a VALE bridge in exclusive mode. See netmap_vale.c for a
	 * description of exclusive mode.
	 * If exclusive mode isn't needed, one can directly attach interfaces to
	 * a VALE bridge through attach_port(). Normal VALE bridges are
	 * automatically created during the attach (and destroyed during the
	 * last detach).
	 */
	auth_token = netmap_bdg_create(MY_BDG, &ret);
	if (ret != 0) {
		D("Error %d while creating bridge %s", ret, MY_BDG);
		return ret;
	}

	ret = create_vale_port(MY_PORT0);
	if (ret != 0) {
		D("Error %d while creating port %s", ret, MY_PORT0);
		return ret;
	}
	ret = create_vale_port(MY_PORT1);
	if (ret != 0) {
		D("Error %d while creating port %s", ret, MY_PORT1);
		return ret;
	}
	ret = create_vale_port(MY_PORT2);
	if (ret != 0) {
		D("Error %d while creating port %s", ret, MY_PORT2);
		return ret;
	}

	ret = attach_port(MY_BDG, MY_PORT0, auth_token, &port_index);
	if (ret != 0) {
		D("Error %d while attaching port %s", ret, MY_PORT0);
		return ret;
	}
	ret = attach_port(MY_BDG, MY_PORT1, auth_token, &port_index);
	if (ret != 0) {
		D("Error %d while attaching port %s", ret, MY_PORT1);
		return ret;
	}
	ret = attach_port(MY_BDG, MY_PORT2, auth_token, &port_index);
	if (ret != 0) {
		D("Error %d while attaching port %s", ret, MY_PORT2);
		return ret;
	}

	/* The third argument is a pointer to a data structure which is passed
	 * to the lookup function.
	 */
	ret = netmap_bdg_regops(MY_BDG, &my_bdg_ops, NULL, auth_token);
	if (ret != 0) {
		D("Error %d while modifying bridge %s", ret, MY_BDG);
		return ret;
	}

	return 0;
}



static void __exit
my_module_fini(void)
{
	int port_index;
	int ret;

	ret = detach_port(MY_BDG, MY_PORT0, auth_token, &port_index);
	if (ret != 0) {
		D("Error %d while detaching port %s", ret, MY_PORT0);
	}
	ret = detach_port(MY_BDG, MY_PORT1, auth_token, &port_index);
	if (ret != 0) {
		D("Error %d while detaching port %s", ret, MY_PORT1);
	}
	ret = detach_port(MY_BDG, MY_PORT2, auth_token, &port_index);
	if (ret != 0) {
		D("Error %d while detaching port %s", ret, MY_PORT2);
	}

	ret = destroy_vale_port(MY_PORT0);
	if (ret != 0) {
		D("Error %d while destroying port %s", ret, MY_PORT0);
	}
	ret = destroy_vale_port(MY_PORT1);
	if (ret != 0) {
		D("Error %d while destroying port %s", ret, MY_PORT1);
	}
	ret = destroy_vale_port(MY_PORT2);
	if (ret != 0) {
		D("Error %d while destroying port %s", ret, MY_PORT2);
	}

	ret = netmap_bdg_destroy(MY_BDG, auth_token);
	if (ret != 0) {
		D("Error %d while destroying bridge %s", ret, MY_BDG);
	}
}



module_init(my_module_init);
module_exit(my_module_fini);
MODULE_AUTHOR("Stefano Duo");
MODULE_DESCRIPTION("VALE minimal module");
MODULE_LICENSE("GPL");