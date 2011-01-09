#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>
#include <inttypes.h>

#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

typedef union cyton_val {
	float 		f;			// float
	double 		d;			// double
	int8_t 		int8; 		// signed char
	uint8_t 	uint8;		// unsigned char
	int16_t 	int16;		// signed short
	uint16_t	uint16;		// unsigned short
	int32_t		int32;		// int 32
	uint32_t 	uint32;		// unsigned int
	int64_t		int64;		// long long
	uint64_t	uint64;		// unsigned long long
} cyton_val;

struct block_device {
	virDomainPtr dom;	/*Domain*/
	char *path;			/*Name of the block device */
};

struct interface_device {
	virDomainPtr dom;	/*Domain*/
	char *path;			/*Name of the interface device */
};

struct domain_info {
	char *domain_name;
	int domain_id;
	unsigned char domain_state;
	unsigned long domain_mem_max;
	unsigned long domain_mem_used;
	unsigned short domain_num_vcpus;
	unsigned long long domain_cpu_total_time;
};

struct domain_disk_info {
	char *domain_name;
	int domain_id;
	unsigned long long domain_vblock_rreq;
	unsigned long long domain_vblock_wreq;
	unsigned long long domain_vblock_rbytes;
	unsigned long long domain_vblock_wbytes;
};

struct domain_interface_info {
	char *domain_name;
	int domain_id;
	unsigned long long domain_if_rxbytes;
	unsigned long long domain_if_txbytes;
	unsigned long long domain_if_rxpackets;
	unsigned long long domain_if_txpackets;
	unsigned long long domain_if_rxerrors;
	unsigned long long domain_if_txerrors;
	unsigned long long domain_if_rxdrops;
	unsigned long long domain_if_txdrops;
};

struct domain_info_list {
	int num_domains;
	struct domain_info *domain_info;
};

struct domain_disk_info_list {
	int num_domains;
	struct domain_disk_info *domain_disk_info;
};

struct domain_interface_info_list {
	int num_domains;
	struct domain_interface_info *domain_interface_info;
};

static virConnectPtr conn = 0;
static virDomainPtr *domains = NULL;
static int nr_domains = 0;

static struct block_device *block_devices = NULL;
static int nr_block_devices = 0;

static struct interface_device *interface_devices = NULL;
static int nr_interface_devices = 0;

static virDomainInfo vir_domain_info; 

int libvirt_init() {
	if(virInitialize() != 0) {
		return -1;
	} else {
		return 0;
	}
}

int libvirt_conn_test() {
	if(libvirt_init() != 0)
		return 1;
	virConnectPtr conn = virConnectOpen("xen:///");
	if(conn == NULL) {
	  printf("libvirt connection open error on uri \n"); 
	  return 1;
	}
	if(conn != NULL) 
		virConnectClose(conn);

	conn = NULL;
	return 0;
}

virConnectPtr libvirt_get_conn() {
	return conn;
}
void free_domains() {
	int i;

	if(domains) {
		for(i = 0; i < nr_domains; ++i) {
			virDomainFree(domains[i]);
		}
		free(domains);
	}
	domains = NULL;
	nr_domains = 0;
}

int add_domain(virDomainPtr dom) {
	virDomainPtr *new_ptr;

	int new_size = sizeof(domains[0]) * (nr_domains + 1);

	if(domains) {
		new_ptr = realloc(domains, new_size);
	} else {
		new_ptr = malloc(new_size);
	}

	if(new_ptr == NULL) 
		return -1;
	
	domains = new_ptr;
	domains[nr_domains] = dom;
	return nr_domains++;
}

void free_block_devices() {
	int i;
	if(block_devices) {
		for(i = 0; i < nr_block_devices; ++i) {
			free(block_devices[i].path);
		}
		block_devices = NULL;
		nr_block_devices = 0;
	}
}

int add_block_device(virDomainPtr dom, const char *path) {
	struct block_device *new_ptr;

	int new_size = sizeof(block_devices[0]) * (nr_block_devices + 1);
	char *path_copy;

	path_copy = strdup(path);
	if(!path_copy)
		return -1;

	if(block_devices)
		new_ptr = realloc(block_devices, new_size);
	else
		new_ptr = malloc(new_size);

	if(new_ptr == NULL) {
		free(path_copy);
		return -1;
	}

	block_devices = new_ptr;
	block_devices[nr_block_devices].dom = dom;
	block_devices[nr_block_devices].path = path_copy;
	return nr_block_devices++;
}

void free_interface_devices() {
	int i;
	if(interface_devices) {
		for(i = 0; i < nr_interface_devices; ++i) {
			free(interface_devices[i].path);
		}
		interface_devices = NULL;
		nr_interface_devices = 0;
	}
}

int add_interface_device(virDomainPtr dom, const char *path) {
	struct interface_device *new_ptr;

	int new_size = sizeof(interface_devices[0]) * (nr_interface_devices + 1);
	char *path_copy;

	path_copy = strdup(path);
	if(!path_copy)
		return -1;

	if(interface_devices)
		new_ptr = realloc(interface_devices, new_size);
	else
		new_ptr = malloc(new_size);

	if(new_ptr == NULL) {
		free(path_copy);
		return -1;
	}

	interface_devices = new_ptr;
	interface_devices[nr_interface_devices].dom = dom;
	interface_devices[nr_interface_devices].path = path_copy;
	return nr_interface_devices++;
}

int libvirt_open() {
	conn = virConnectOpen("xen:///");
	if(conn == NULL) {
	  printf("libvirt connection open error on uri \n"); 
	  return 0;
	}
	int n;
	n = virConnectNumOfDomains(conn);
	if(n < 0) {
		printf("Error reading number of domains \n");
		return -1;
	}

	int i;
	int *domids;

	domids = malloc(sizeof(int) * n);
	if(domids == 0) {
		printf("libvirt domain ids malloc failed");
		return -1;
	}
	n = virConnectListDomains(conn, domids, n);
	if(n < 0) {
		printf("Error reading list of domains \n");
		free(domids);
		return -1;
	}

	free_block_devices();
	free_interface_devices();
	free_domains();

	for (i = 0; i < n ; ++i) {
		virDomainPtr dom = NULL;
		const char *name;
		char *xml = NULL;
		xmlDocPtr xml_doc = NULL;
		xmlXPathContextPtr xpath_ctx = NULL;
		xmlXPathObjectPtr xpath_obj = NULL;
		int j;

		//printf("Publishing Domain Id : %d \n ", domids[i]);
		dom = virDomainLookupByID(conn, domids[i]);
		if(dom == NULL) {
			printf("Domain no longer active or moved away .. \n");
		}
		name = virDomainGetName(dom);
		//printf("Publishing Domain Name : %s \n ", name);
		if(name == NULL) {
			printf("Domain name not valid .. \n");
			goto cont;	
		}
		if(add_domain(dom) < 0) {
			printf("libvirt plugin malloc failed .. \n");
			goto cont;
		}
		xml = virDomainGetXMLDesc(dom, 0);
		if(!xml) {
			printf("Virt domain xml description error ..\n");
			goto cont;
		}
		//printf("Publishing XML : \n %s \n ", xml);

		xml_doc = xmlReadDoc((xmlChar *) xml, NULL, NULL, XML_PARSE_NONET);
		if(xml_doc == NULL) {
			printf("XML read doc error ..\n");
			goto cont;
		}

		xpath_ctx = xmlXPathNewContext(xml_doc);
		xpath_obj = xmlXPathEval((xmlChar *) "/domain/devices/disk/target[@dev]", xpath_ctx);
		if(xpath_obj == NULL || xpath_obj->type != XPATH_NODESET || xpath_obj->nodesetval == NULL) {
			goto cont;
		}

		for(j = 0; j < xpath_obj->nodesetval->nodeNr; ++j) {
			xmlNodePtr node;
			char *path = NULL;
			node = xpath_obj->nodesetval->nodeTab[j];
			if(!node) continue;
			path = (char *) xmlGetProp (node, (xmlChar *) "dev");
			if(!path) continue;
			add_block_device(dom, path);
		}
		xmlXPathFreeObject(xpath_obj);

		xpath_obj = xmlXPathEval((xmlChar *) "/domain/devices/interface/target[@dev]", xpath_ctx);
		if(xpath_obj == NULL || xpath_obj->type != XPATH_NODESET || xpath_obj->nodesetval == NULL) {
			goto cont;
		}

		for(j = 0; j < xpath_obj->nodesetval->nodeNr; ++j) {
			xmlNodePtr node;
			char *path = NULL;
			node = xpath_obj->nodesetval->nodeTab[j];
			if(!node) continue;
			path = (char *) xmlGetProp(node, (xmlChar *) "dev");
			if(!path) continue;
			add_interface_device(dom, path);
		}
		cont:
			if(xpath_obj) xmlXPathFreeObject(xpath_obj);
			if(xpath_ctx) xmlXPathFreeContext(xpath_ctx);
			if(xml_doc) xmlFreeDoc(xml_doc);
			if(xml) free(xml);
	}
	free(domids);
	return 0;
}

int libvirt_close() {
	free_block_devices();
	free_interface_devices();
	free_domains();
	if(conn != NULL)
		virConnectClose(conn);
	conn = NULL;
}

int libvirt_read_stats() {
	int i;

	printf("Num Domains : %d .. \n", nr_domains);
	for(i = 0; i < nr_domains; ++i) {
		//virDomainInfo info; 
		virVcpuInfoPtr vinfo = NULL;
		int j;
		
		if(virDomainGetInfo(domains[i], &vir_domain_info) != 0) 
			continue;

		vinfo = malloc(vir_domain_info.nrVirtCpu * sizeof vinfo[0]);
		if(vinfo == NULL){
			printf("libvirt plugin malloc failed ..\n");
			continue;
		}
		
		if(virDomainGetVcpus(domains[i], vinfo, vir_domain_info.nrVirtCpu, NULL, 0) != 0) {
			free(vinfo);
			continue;
		}

		for(j = 0; j < vir_domain_info.nrVirtCpu; j++) {
			printf("VCPU number : %u .. \n", (unsigned int) vinfo[j].number);
			printf("VCPU time : %llu .. \n", (unsigned long long) vinfo[j].cpuTime);
		}
		free(vinfo);
	}

	printf("Num Block Devices : %d .. \n", nr_block_devices);
	for(i = 0; i < nr_block_devices; ++i) {
		struct _virDomainBlockStats stats;
		if(virDomainBlockStats(block_devices[i].dom, block_devices[i].path, &stats, sizeof stats) != 0)
			continue;
		if((stats.rd_req != -1) && (stats.wr_req != -1)) {
			printf("Read reqs : %llu \n", (unsigned long long) stats.rd_req);
			printf("Write reqs : %llu \n", (unsigned long long) stats.wr_req);
		}
		if((stats.rd_bytes != -1) && (stats.rd_bytes != -1)) {
			printf("Read bytes : %llu \n", (unsigned long long) stats.rd_bytes);
			printf("Write bytes : %llu \n", (unsigned long long) stats.wr_bytes);
		}
	}

	printf("Num Interface Devices : %d .. \n", nr_interface_devices);
	for(i = 0; i < nr_interface_devices; ++i) {
		struct _virDomainInterfaceStats stats;
		if(virDomainInterfaceStats(interface_devices[i].dom, interface_devices[i].path, &stats, sizeof stats) != 0)
			continue;
		if((stats.rx_bytes != -1) && (stats.tx_bytes != -1)) {
			printf("Recieved bytes : %llu \n", (unsigned long long) stats.rx_bytes);
			printf("Transmitted bytes : %llu \n", (unsigned long long) stats.tx_bytes);
		}
		if((stats.rx_packets != -1) && (stats.tx_packets != -1)) {
			printf("Recieved packets : %llu \n", (unsigned long long) stats.rx_packets);
			printf("Transmitted packets : %llu \n",(unsigned long long) stats.tx_packets);
		}
		if((stats.rx_errs != -1) && (stats.tx_errs != -1)) {
			printf("Recieved errors : %llu \n", (unsigned long long) stats.rx_errs);
			printf("Transmitted errors : %llu \n", (unsigned long long) stats.tx_errs);
		}
		if((stats.rx_drop != -1) && (stats.tx_drop != -1)) {
			printf("Recieved dropped : %ll \n", (unsigned long long) stats.rx_drop);
			printf("Transmitted dropped : %ll \n", (unsigned long long) stats.tx_drop);
		}
	}
}

int libvirt_num_domains() {
	return nr_domains;
}

int libvirt_num_active_domains() {
	int num_domains = virConnectNumOfDomains(conn);
	return num_domains;
}

int libvirt_num_inactive_domains() {
	int num_domains = virConnectNumOfDefinedDomains(conn);
	return num_domains;
}

struct domain_info_list *libvirt_domain_info_list() {
	int i;

	struct domain_info_list *domain_info_list = (struct domain_info_list *) calloc(1, sizeof(struct domain_info_list));
	domain_info_list->num_domains = nr_domains;
	domain_info_list->domain_info = (struct domain_info *) calloc(nr_domains, sizeof(struct domain_info));

	for(i = 0; i < nr_domains; i++) {
		if(virDomainGetInfo(domains[i], &vir_domain_info) != 0) 
			continue;

		domain_info_list->domain_info[i].domain_name = strdup(virDomainGetName(domains[i]));
		domain_info_list->domain_info[i].domain_id = virDomainGetID(domains[i]);
		domain_info_list->domain_info[i].domain_state = vir_domain_info.state;
		domain_info_list->domain_info[i].domain_mem_max = vir_domain_info.maxMem;
		domain_info_list->domain_info[i].domain_mem_used = vir_domain_info.memory;
		domain_info_list->domain_info[i].domain_num_vcpus = vir_domain_info.nrVirtCpu;
		domain_info_list->domain_info[i].domain_cpu_total_time = vir_domain_info.cpuTime;
	}
	return domain_info_list;
}

struct domain_disk_info_list *libvirt_domain_disk_info_list() {
	int i;
	int j = 0;
	unsigned long long vblock_rreq = 0L;
	unsigned long long vblock_wreq = 0L;
	unsigned long long vblock_rbytes = 0L;
	unsigned long long vblock_wbytes = 0L;

	struct domain_disk_info_list *domain_disk_info_list = (struct domain_disk_info_list *) calloc(1, sizeof(struct domain_disk_info_list));
	domain_disk_info_list->num_domains = nr_domains;
	domain_disk_info_list->domain_disk_info = (struct domain_disk_info *) calloc(nr_domains, sizeof(struct domain_disk_info));

	for(i = 0; i < nr_block_devices; i++) {
		struct _virDomainBlockStats stats;
		if(virDomainBlockStats(block_devices[i].dom, block_devices[i].path, &stats, sizeof stats) != 0)
			continue;
		vblock_rreq += (unsigned long long) stats.rd_req;
		vblock_wreq += (unsigned long long) stats.wr_req;
		vblock_rbytes += (unsigned long long) stats.rd_bytes;
		vblock_wbytes += (unsigned long long) stats.wr_bytes;

		if(((i+1)%2) == 0) {
			if(j < nr_domains) {
				domain_disk_info_list->domain_disk_info[j].domain_name = strdup(virDomainGetName(domains[j+1]));
				domain_disk_info_list->domain_disk_info[j].domain_id = virDomainGetID(domains[j+1]);
				domain_disk_info_list->domain_disk_info[j].domain_vblock_rreq = vblock_rreq;
				domain_disk_info_list->domain_disk_info[j].domain_vblock_wreq = vblock_wreq;
				domain_disk_info_list->domain_disk_info[j].domain_vblock_rbytes = vblock_rbytes;
				domain_disk_info_list->domain_disk_info[j].domain_vblock_wbytes = vblock_wbytes;
				j += 1;
			}
			vblock_rreq = 0L;
			vblock_wreq = 0L;
			vblock_rbytes = 0L;
			vblock_wbytes = 0L;
		} 
	}
	return domain_disk_info_list;
}

struct domain_interface_info_list *libvirt_domain_interface_info_list() {
	int i;
	int j = 0;
	unsigned long long if_rxbytes = 0L;
	unsigned long long if_txbytes = 0L;
	unsigned long long if_rxpackets = 0L;
	unsigned long long if_txpackets = 0L;
	unsigned long long if_rxerrors = 0L;
	unsigned long long if_txerrors = 0L;
	unsigned long long if_rxdrops = 0L;
	unsigned long long if_txdrops = 0L;

	struct domain_interface_info_list *domain_interface_info_list = (struct domain_interface_info_list *) calloc(1, 
	sizeof(struct domain_interface_info_list));
	domain_interface_info_list->num_domains = nr_domains;
	domain_interface_info_list->domain_interface_info = (struct domain_interface_info *) calloc(nr_domains, sizeof(struct domain_interface_info));

	for(i = 0; i < nr_interface_devices; i++) {
		struct _virDomainInterfaceStats stats;
		if(virDomainInterfaceStats(interface_devices[i].dom, interface_devices[i].path, &stats, sizeof stats) != 0)
			continue;

		if_rxbytes += (unsigned long long) stats.rx_bytes;
		if_txbytes += (unsigned long long) stats.tx_bytes;
		if_rxpackets += (unsigned long long) stats.rx_packets;
		if_txpackets += (unsigned long long) stats.tx_packets;
		if_rxerrors += (unsigned long long) stats.rx_errs;
		if_txerrors += (unsigned long long) stats.tx_errs;
		if_rxdrops += (unsigned long long) stats.rx_drop;
		if_txdrops += (unsigned long long) stats.tx_drop;

		if(j < nr_domains) {
			domain_interface_info_list->domain_interface_info[j].domain_name = strdup(virDomainGetName(domains[j+1]));
			domain_interface_info_list->domain_interface_info[j].domain_id = virDomainGetID(domains[j+1]);
			domain_interface_info_list->domain_interface_info[j].domain_if_rxbytes = if_rxbytes;
			domain_interface_info_list->domain_interface_info[j].domain_if_txbytes = if_txbytes;
			domain_interface_info_list->domain_interface_info[j].domain_if_rxpackets = if_rxpackets;
			domain_interface_info_list->domain_interface_info[j].domain_if_txpackets = if_txpackets;
			domain_interface_info_list->domain_interface_info[j].domain_if_rxerrors = if_rxerrors;
			domain_interface_info_list->domain_interface_info[j].domain_if_txerrors = if_txerrors;
			domain_interface_info_list->domain_interface_info[j].domain_if_rxdrops = if_rxdrops;
			domain_interface_info_list->domain_interface_info[j].domain_if_txdrops = if_txdrops;
			j += 1;
		}
		if_rxbytes = 0L;
		if_txbytes = 0L;
		if_rxpackets = 0L;
		if_txpackets = 0L;
		if_rxerrors = 0L;
		if_txerrors = 0L;
		if_rxdrops = 0L;
		if_txdrops = 0L;
	}
	return domain_interface_info_list;
}

struct domain_info *libvirt_get_domain_info(char *domain_name) {
	int i;
	struct domain_info_list *domain_info_list = libvirt_domain_info_list();
	struct domain_info *domain_info = (struct domain_info *) calloc(1, sizeof(struct domain_info));

	for(i = 0; i < domain_info_list->num_domains; i++) {
		if(strstr(domain_info_list->domain_info[i].domain_name, domain_name)) {
			domain_info->domain_name = strdup(domain_info_list->domain_info[i].domain_name);
			domain_info->domain_id = domain_info_list->domain_info[i].domain_id;
			domain_info->domain_state = domain_info_list->domain_info[i].domain_state;
			domain_info->domain_mem_max = domain_info_list->domain_info[i].domain_mem_max;
			domain_info->domain_mem_used = domain_info_list->domain_info[i].domain_mem_used;
			domain_info->domain_num_vcpus = domain_info_list->domain_info[i].domain_num_vcpus;
			domain_info->domain_cpu_total_time = domain_info_list->domain_info[i].domain_cpu_total_time;
			break;
		}
	}
	return domain_info;
}

struct domain_disk_info *libvirt_get_domain_disk_info(char *domain_name) {
	int i;
	struct domain_disk_info_list *domain_disk_info_list = libvirt_domain_disk_info_list();
	struct domain_disk_info *domain_disk_info = (struct domain_disk_info *) calloc(1, sizeof(struct domain_disk_info));

	for(i = 0; i < domain_disk_info_list->num_domains; i++) {
		if(strstr(domain_disk_info_list->domain_disk_info[i].domain_name, domain_name)) {
			domain_disk_info->domain_name = strdup(domain_disk_info_list->domain_disk_info[i].domain_name);
			domain_disk_info->domain_id = domain_disk_info_list->domain_disk_info[i].domain_id;
			domain_disk_info->domain_vblock_rreq = domain_disk_info_list->domain_disk_info[i].domain_vblock_rreq;
			domain_disk_info->domain_vblock_wreq = domain_disk_info_list->domain_disk_info[i].domain_vblock_wreq;
			domain_disk_info->domain_vblock_rbytes = domain_disk_info_list->domain_disk_info[i].domain_vblock_rbytes;
			domain_disk_info->domain_vblock_wbytes = domain_disk_info_list->domain_disk_info[i].domain_vblock_wbytes;
			break;
		}
	}
	return domain_disk_info;
}

struct domain_interface_info *libvirt_get_domain_interface_info(char *domain_name) {
	int i;
	struct domain_interface_info_list *domain_interface_info_list = libvirt_domain_interface_info_list();
	struct domain_interface_info *domain_interface_info = (struct domain_interface_info *) calloc(1, sizeof(struct domain_interface_info));

	for(i = 0; i < domain_interface_info_list->num_domains; i++) {
		if(strstr(domain_interface_info_list->domain_interface_info[i].domain_name, domain_name)) {
			domain_interface_info->domain_name = strdup(domain_interface_info_list->domain_interface_info[i].domain_name);
			domain_interface_info->domain_id = domain_interface_info_list->domain_interface_info[i].domain_id;
			domain_interface_info->domain_if_rxbytes = domain_interface_info_list->domain_interface_info[i].domain_if_rxbytes;
			domain_interface_info->domain_if_txbytes = domain_interface_info_list->domain_interface_info[i].domain_if_txbytes;
			domain_interface_info->domain_if_rxpackets = domain_interface_info_list->domain_interface_info[i].domain_if_rxpackets;
			domain_interface_info->domain_if_txpackets = domain_interface_info_list->domain_interface_info[i].domain_if_txpackets;
			domain_interface_info->domain_if_rxerrors = domain_interface_info_list->domain_interface_info[i].domain_if_rxerrors;
			domain_interface_info->domain_if_txerrors = domain_interface_info_list->domain_interface_info[i].domain_if_txerrors;
			domain_interface_info->domain_if_rxdrops = domain_interface_info_list->domain_interface_info[i].domain_if_rxdrops;
			domain_interface_info->domain_if_txdrops = domain_interface_info_list->domain_interface_info[i].domain_if_txdrops;
			break;
		}
	}
	return domain_interface_info;
}

int libvirt_domain_exists(char *domain_name) {
	int i; int domain_exists = 0;
	struct domain_info_list *domain_info_list = libvirt_domain_info_list();

	for(i = 0; i < domain_info_list->num_domains; i++) {
		if(strstr(domain_info_list->domain_info[i].domain_name, domain_name)) {
			domain_exists = 1;	
			break;
		}
	}
	return domain_exists;
}

int libvirt_get_domain_id(char *domain_name) {
	int i; int domain_id = -1;
	struct domain_info_list *domain_info_list = libvirt_domain_info_list();

	//printf("Num of Domains : %d ", domain_info_list->num_domains);
	for(i = 0; i < domain_info_list->num_domains; i++) {
		//printf("Domain name %d in the info list : %s ", i, domain_info_list->domain_info[i].domain_name);
		if(strstr(domain_info_list->domain_info[i].domain_name, domain_name)) {
			domain_id = domain_info_list->domain_info[i].domain_id;	
			break;
		}
	}
	return domain_id;
}

unsigned char libvirt_get_domain_state(char *domain_name) {
	int i; unsigned char domain_state = -1;
	struct domain_info_list *domain_info_list = libvirt_domain_info_list();

	for(i = 0; i < domain_info_list->num_domains; i++) {
		if(strstr(domain_info_list->domain_info[i].domain_name, domain_name)) {
			domain_state = domain_info_list->domain_info[i].domain_state;	
			break;
		}
	}
	return domain_state;
}

unsigned long libvirt_get_domain_mem_max(char *domain_name) {
	int i; unsigned long domain_mem_max = -1;
	struct domain_info_list *domain_info_list = libvirt_domain_info_list();

	for(i = 0; i < domain_info_list->num_domains; i++) {
		if(strstr(domain_info_list->domain_info[i].domain_name, domain_name)) {
			domain_mem_max = domain_info_list->domain_info[i].domain_mem_max;	
			break;
		}
	}
	return domain_mem_max;
}

unsigned long libvirt_get_domain_mem_used(char *domain_name) {
	int i; unsigned long domain_mem_used = -1;
	struct domain_info_list *domain_info_list = libvirt_domain_info_list();

	for(i = 0; i < domain_info_list->num_domains; i++) {
		if(strstr(domain_info_list->domain_info[i].domain_name, domain_name)) {
			domain_mem_used = domain_info_list->domain_info[i].domain_mem_used;	
			break;
		}
	}
	return domain_mem_used;
}

unsigned short libvirt_get_domain_num_vcpus(char *domain_name) {
	int i; unsigned short domain_num_vcpus = -1;
	struct domain_info_list *domain_info_list = libvirt_domain_info_list();

	for(i = 0; i < domain_info_list->num_domains; i++) {
		if(strstr(domain_info_list->domain_info[i].domain_name, domain_name)) {
			domain_num_vcpus = domain_info_list->domain_info[i].domain_num_vcpus;	
			break;
		}
	}
	return domain_num_vcpus;
}

unsigned long libvirt_get_domain_cpu_total_time(char *domain_name) {
	int i; unsigned long long domain_cpu_total_time = -1;
	struct domain_info_list *domain_info_list = libvirt_domain_info_list();

	for(i = 0; i < domain_info_list->num_domains; i++) {
		if(strstr(domain_info_list->domain_info[i].domain_name, domain_name)) {
			domain_cpu_total_time = domain_info_list->domain_info[i].domain_cpu_total_time;	
			break;
		}
	}
	return (unsigned long)domain_cpu_total_time;
}

unsigned long libvirt_get_domain_vblock_rreq(char *domain_name) {
	int i; unsigned long long domain_vblock_rreq = -1;
	struct domain_disk_info_list *domain_disk_info_list = libvirt_domain_disk_info_list();

	for(i = 0; i < domain_disk_info_list->num_domains; i++) {
		if(strstr(domain_disk_info_list->domain_disk_info[i].domain_name, domain_name)) {
			domain_vblock_rreq = domain_disk_info_list->domain_disk_info[i].domain_vblock_rreq;	
			break;
		}
	}
	return (unsigned long)domain_vblock_rreq;
}

unsigned long libvirt_get_domain_vblock_wreq(char *domain_name) {
	int i; unsigned long long domain_vblock_wreq = -1;
	struct domain_disk_info_list *domain_disk_info_list = libvirt_domain_disk_info_list();

	for(i = 0; i < domain_disk_info_list->num_domains; i++) {
		if(strstr(domain_disk_info_list->domain_disk_info[i].domain_name, domain_name)) {
			domain_vblock_wreq = domain_disk_info_list->domain_disk_info[i].domain_vblock_wreq;	
			break;
		}
	}
	return (unsigned long)domain_vblock_wreq;
}

unsigned long libvirt_get_domain_vblock_rbytes(char *domain_name) {
	int i; unsigned long long domain_vblock_rbytes = -1;
	struct domain_disk_info_list *domain_disk_info_list = libvirt_domain_disk_info_list();

	for(i = 0; i < domain_disk_info_list->num_domains; i++) {
		if(strstr(domain_disk_info_list->domain_disk_info[i].domain_name, domain_name)) {
			domain_vblock_rbytes = domain_disk_info_list->domain_disk_info[i].domain_vblock_rbytes;	
			break;
		}
	}
	return (unsigned long)domain_vblock_rbytes;
}

unsigned long libvirt_get_domain_vblock_wbytes(char *domain_name) {
	int i; unsigned long long domain_vblock_wbytes = -1;
	struct domain_disk_info_list *domain_disk_info_list = libvirt_domain_disk_info_list();

	for(i = 0; i < domain_disk_info_list->num_domains; i++) {
		if(strstr(domain_disk_info_list->domain_disk_info[i].domain_name, domain_name)) {
			domain_vblock_wbytes = domain_disk_info_list->domain_disk_info[i].domain_vblock_wbytes;	
			break;
		}
	}
	return (unsigned long)domain_vblock_wbytes;
}

unsigned long libvirt_get_domain_if_rxbytes(char *domain_name) {
	int i; unsigned long long domain_if_rxbytes = -1;
	struct domain_interface_info_list *domain_interface_info_list = libvirt_domain_interface_info_list();

	for(i = 0; i < domain_interface_info_list->num_domains; i++) {
		if(strstr(domain_interface_info_list->domain_interface_info[i].domain_name, domain_name)) {
			domain_if_rxbytes = domain_interface_info_list->domain_interface_info[i].domain_if_rxbytes;	
			break;
		}
	}
	return (unsigned long)domain_if_rxbytes;
}

unsigned long libvirt_get_domain_if_txbytes(char *domain_name) {
	int i; unsigned long long domain_if_txbytes = -1;
	struct domain_interface_info_list *domain_interface_info_list = libvirt_domain_interface_info_list();

	for(i = 0; i < domain_interface_info_list->num_domains; i++) {
		if(strstr(domain_interface_info_list->domain_interface_info[i].domain_name, domain_name)) {
			domain_if_txbytes = domain_interface_info_list->domain_interface_info[i].domain_if_txbytes;	
			break;
		}
	}
	return (unsigned long)domain_if_txbytes;
}

unsigned long libvirt_get_domain_if_rxpackets(char *domain_name) {
	int i; unsigned long long domain_if_rxpackets = -1;
	struct domain_interface_info_list *domain_interface_info_list = libvirt_domain_interface_info_list();

	for(i = 0; i < domain_interface_info_list->num_domains; i++) {
		if(strstr(domain_interface_info_list->domain_interface_info[i].domain_name, domain_name)) {
			domain_if_rxpackets = domain_interface_info_list->domain_interface_info[i].domain_if_rxpackets;	
			break;
		}
	}
	return (unsigned long)domain_if_rxpackets;
}

unsigned long libvirt_get_domain_if_txpackets(char *domain_name) {
	int i; unsigned long long domain_if_txpackets = -1;
	struct domain_interface_info_list *domain_interface_info_list = libvirt_domain_interface_info_list();

	for(i = 0; i < domain_interface_info_list->num_domains; i++) {
		if(strstr(domain_interface_info_list->domain_interface_info[i].domain_name, domain_name)) {
			domain_if_txpackets = domain_interface_info_list->domain_interface_info[i].domain_if_txpackets;	
			break;
		}
	}
	return (unsigned long)domain_if_txpackets;
}

unsigned long libvirt_get_domain_if_rxerrors(char *domain_name) {
	int i; unsigned long long domain_if_rxerrors = -1;
	struct domain_interface_info_list *domain_interface_info_list = libvirt_domain_interface_info_list();

	for(i = 0; i < domain_interface_info_list->num_domains; i++) {
		if(strstr(domain_interface_info_list->domain_interface_info[i].domain_name, domain_name)) {
			domain_if_rxerrors = domain_interface_info_list->domain_interface_info[i].domain_if_rxerrors;	
			break;
		}
	}
	return (unsigned long)domain_if_rxerrors;
}

unsigned long libvirt_get_domain_if_txerrors(char *domain_name) {
	int i; unsigned long long domain_if_txerrors = -1;
	struct domain_interface_info_list *domain_interface_info_list = libvirt_domain_interface_info_list();

	for(i = 0; i < domain_interface_info_list->num_domains; i++) {
		if(strstr(domain_interface_info_list->domain_interface_info[i].domain_name, domain_name)) {
			domain_if_txerrors = domain_interface_info_list->domain_interface_info[i].domain_if_txerrors;	
			break;
		}
	}
	return (unsigned long)domain_if_txerrors;
}

unsigned long libvirt_get_domain_if_rxdrops(char *domain_name) {
	int i; unsigned long long domain_if_rxdrops = -1;
	struct domain_interface_info_list *domain_interface_info_list = libvirt_domain_interface_info_list();

	for(i = 0; i < domain_interface_info_list->num_domains; i++) {
		if(strstr(domain_interface_info_list->domain_interface_info[i].domain_name, domain_name)) {
			domain_if_rxdrops = domain_interface_info_list->domain_interface_info[i].domain_if_rxdrops;	
			break;
		}
	}
	return (unsigned long)domain_if_rxdrops;
}

unsigned long libvirt_get_domain_if_txdrops(char *domain_name) {
	int i; unsigned long long domain_if_txdrops = -1;
	struct domain_interface_info_list *domain_interface_info_list = libvirt_domain_interface_info_list();

	for(i = 0; i < domain_interface_info_list->num_domains; i++) {
		if(strstr(domain_interface_info_list->domain_interface_info[i].domain_name, domain_name)) {
			domain_if_txdrops = domain_interface_info_list->domain_interface_info[i].domain_if_txdrops;	
			break;
		}
	}
	return (unsigned long)domain_if_txdrops;
}

int libvirt_set_vcpus(char *domain_name, int num_vcpus) {
	int i;
	for(i = 0; i < nr_domains; i++) {
		if(strstr(domain_name, virDomainGetName(domains[i]))) {
			return virDomainSetVcpus(domains[i], num_vcpus);
		}
	}
}

int libvirt_set_memory(char *domain_name, unsigned long mem_kbs) {
	int i;
	for(i = 0; i < nr_domains; i++) {
		if(strstr(domain_name, virDomainGetName(domains[i]))) {
			return virDomainSetMemory(domains[i], mem_kbs);
		}
	}
}

int libvirt_create_domain(const char *xmlDesc) {
	virConnectPtr conn = libvirt_get_conn();
	virDomainPtr newDomU = virDomainCreateXML(conn, xmlDesc, 0);
	if(newDomU) {
		return 1;
	} else {
		return 0;
	}
}

int libvirt_shutdown_domain(char *domain_name) {
	int domUid = libvirt_get_domain_id(domain_name); 
	//printf("Domain id of DomU to shutdown : %d \n", domUid);
	virConnectPtr conn = libvirt_get_conn();
	virDomainPtr domU = virDomainLookupByID(conn, domUid);
	int shut = virDomainShutdown(domU);
	if(!shut) {
		return 1;
	} else {
		return 0;
	}
}

int libvirt_sleep(int time) {
	sleep(time);
	return time;
}


//int main(int argc, char **argv) {
	//printf (" Test %  ..\n");
	//libvirt_init();
	//libvirt_open();
	//libvirt_read_stats();	
	/*		
	printf("Domain info - name  : %s \n", libvirt_get_domain_info("apache")->domain_name); 
	printf("Domain info - vblock_rreq   : %llu \n", libvirt_get_domain_vblock_rreq("apache")); 
	printf("Domain info - vblock_wreq   : %llu \n", libvirt_get_domain_vblock_wreq("apache")); 
	printf("Domain info - vblock_rbytes : %llu \n", libvirt_get_domain_vblock_rbytes("apache")); 
	printf("Domain info - vblock_wbytes : %llu \n", libvirt_get_domain_vblock_wbytes("apache"));
	*/
	/*
	printf("Domain info - name  : %s \n", libvirt_get_domain_info("tomcat")->domain_name); 
	printf("Domain info - vblock_rreq   : %llu \n", libvirt_get_domain_vblock_rreq("tomcat")); 
	printf("Domain info - vblock_wreq   : %llu \n", libvirt_get_domain_vblock_wreq("tomcat")); 
	printf("Domain info - vblock_rbytes : %llu \n", libvirt_get_domain_vblock_rbytes("tomcat")); 
	printf("Domain info - vblock_wbytes : %llu \n", libvirt_get_domain_vblock_wbytes("tomcat")); 
	*/
	/*
	printf("Domain info - name  : %s \n", libvirt_get_domain_info("apache")->domain_name); 
	printf("Domain info - if_txpackets : %llu \n", libvirt_get_domain_if_txpackets("apache")); 
	printf("Domain info - if_rxpackets : %llu \n", libvirt_get_domain_if_rxpackets("apache")); 
	printf("Domain info - if_txbytes : %llu \n", libvirt_get_domain_if_txbytes("apache")); 
	printf("Domain info - if_rxbytes : %llu \n", libvirt_get_domain_if_rxbytes("apache")); 
	printf("Domain info - if_txerrors : %llu \n", libvirt_get_domain_if_txerrors("apache")); 
	printf("Domain info - if_rxerrors : %llu \n", libvirt_get_domain_if_rxerrors("apache")); 
	printf("Domain info - if_txdrops : %llu \n", libvirt_get_domain_if_txdrops("apache")); 
	printf("Domain info - if_rxdrops : %llu \n", libvirt_get_domain_if_rxdrops("apache"));
	*/
	/*
	printf("Domain Domain-0 info - num vpcus %u \n", libvirt_get_domain_num_vcpus("Domain-0")); 
	printf("Domain Domain-0 info - mem max %lu \n", libvirt_get_domain_mem_max("Domain-0")); 
	printf("Domain Domain-0 info - mem used %lu \n", libvirt_get_domain_mem_used("Domain-0")); 
	printf("Domain Domain-0 info - total cpu time %lu \n", libvirt_get_domain_cpu_total_time("Domain-0"));

	printf("Domain Apache info - total cpu time %lu \n", libvirt_get_domain_cpu_total_time("apache"));
	printf("Domain Apache info - if_txbytes : %llu \n", libvirt_get_domain_if_txbytes("apache")); 
	printf("Domain Apache info - if_rxbytes : %llu \n", libvirt_get_domain_if_rxbytes("apache")); 
	
	printf("Domain Tomcat info - total cpu time %lu \n", libvirt_get_domain_cpu_total_time("tomcat"));
	printf("Domain Tomcat info - if_txbytes : %llu \n", libvirt_get_domain_if_txbytes("tomcat")); 
	printf("Domain Tomcat info - if_rxbytes : %llu \n", libvirt_get_domain_if_rxbytes("tomcat")); 

	printf("Domain MySQL info - total cpu time %lu \n", libvirt_get_domain_cpu_total_time("mysql"));
	printf("Domain MySQL info - if_txbytes : %llu \n", libvirt_get_domain_if_txbytes("mysql")); 
	printf("Domain MySQL info - if_rxbytes : %llu \n", libvirt_get_domain_if_rxbytes("mysql")); 
	*/

	//\t<uuid>670fda74-539f-da77-64d7-9916ec2eb801</uuid>\n\
    //<interface type='bridge'>\n\
    //<mac address='00:16:3e:34:34:34'/>\n\
    //<source bridge='xenbr1'/>\n\
    //<ip address='10.0.0.134'/>\n\
	//<script path='vif-bridge bridge=xenbr1'/>\n\
    //<target dev='vif34.0'/>\n\
	//</interface>\n\
	//<domain type='xen' id='34'>\n\
	
	/*
	char *xml = "\n\
	<domain type='xen'>\n\
	<name>mapachenew</name>\n\
	<memory>524288</memory>\n\
	<currentMemory>262144</currentMemory>\n\
	<vcpu>2</vcpu>\n\
	<os>\n\
	<type>linux</type>\n\
	<kernel>/var/local/mahendra/common/vmlinuz-3.4.1-2.6.18.8-xenU</kernel>\n\
	<initrd>/var/local/mahendra/common/initrd.img-2.6.18.8-xen0</initrd>\n\
	<cmdline>xencons=xvc console=xvc0</cmdline>\n\
	<root>/dev/hda1 ro</root>\n\
	</os>\n\
	<clock offset='utc'/>\n\
	<on_poweroff>destroy</on_poweroff>\n\
	<on_reboot>restart</on_reboot>\n\
	<on_crash>restart</on_crash>\n\
	<devices>\n\
	<disk type='file' device='disk'>\n\
	<driver name='file'/>\n\
	<source file='/var/local/mahendra/apache-httpd/newapachehttpd.img'/>\n\
	<target dev='hda1' bus='ide'/>\n\
	</disk>\n\
	<disk type='file' device='disk'>\n\
	<driver name='file'/>\n\
	<source file='/var/local/mahendra/apache-httpd/newapachehttpd-swap.img'/>\n\
	<target dev='hda2' bus='ide'/>\n\
    </disk>\n\
    <interface type='bridge'>\n\
    <mac address='00:16:3e:34:34:34'/>\n\
    <source bridge='xenbr1'/>\n\
    <ip address='10.0.0.134'/>\n\
	<script path='vif-bridge bridge=xenbr1'/>\n\
    <target dev='vif34.0'/>\n\
	</interface>\n\
	<console type='pty' tty='/dev/pts/3'>\n\
    <source path='/dev/pts/3'/>\n\
	<target port='0'/>\n\
    </console>\n\
    </devices>\n\
	</domain>\n\
	";

	int ret = libvirt_shutdown_domain("mapache");
	printf("Return shutdown value %d \n ", ret);

	//int retsleep = libvirt_sleep(10);
	//printf("Return sleep value %d \n ", retsleep);

	//libvirt_close();
	//libvirt_open();

	int retcreate = libvirt_create_domain(xml);
	printf("Return create value %d \n ", retcreate);

	//int ret = libvirt_shutdown_domain("apache");
	//printf("Return value %d ", ret);

	//libvirt_set_vpcus("Domain-0", 7);
	//uint32_t mem = 1024*1024; //Can add up to 16GB 16777216 Kbs = 16*1024*1024
	//libvirt_set_memory("Domain-0", mem);
	//libvirt_close();
}*/
