#include "metricssample.h"

char * skip_whitespace (const char *p) {
    while (isspace((unsigned char)*p)) p++;
    return (char *)p;
}
 
char * skip_token (const char *p) {
    while (isspace((unsigned char)*p)) p++;
    while (*p && !isspace((unsigned char)*p)) p++;
    return (char *)p;
}         

int slurpfile(char *filename, char *buffer, int buflen) {
	int fd, read_len;

	fd = open(filename, O_RDONLY);
	if(fd < 0) {
	  printf("open() error on file %s \n", filename); 
	  exit(0);
	}
	read_len = read(fd, buffer, buflen);
	if(read_len <= 0) {
	  printf("read() error on file %s \n", filename); 
	  exit(0);
	}
	if (read_len == buflen) {
          --read_len;
         printf("slurpfile() read() buffer overflow on file %s", filename);
    }
    buffer[read_len] = '\0';
	close(fd);
	return read_len;
}

char *update_file(sensor_slurp *sf) {
	int rval = slurpfile(sf->name, sf->buffer, SAMPLEBUFFERSIZE);
	return sf->buffer;
}

unsigned int num_cpustates_func ( void ) {
   char *p;
   unsigned int i=0;

   p = update_file(&proc_stat);

/*
** Skip initial "cpu" token
*/
   p = skip_token(p);
   p = skip_whitespace(p);
/*
** Loop over file until next "cpu" token is found.
** i=4 : Linux 2.4.x
** i=7 : Linux 2.6.x
*/
   while (strncmp(p,"cpu",3)) {
     p = skip_token(p);
     p = skip_whitespace(p);
     i++;
     }

   return i;
}

unsigned long total_jiffies_func ( void ) {

   char *p;
   unsigned long user_jiffies, nice_jiffies, system_jiffies, idle_jiffies,
                 wio_jiffies, irq_jiffies, sirq_jiffies;

   p = update_file(&proc_stat);
   p = skip_token(p);
   p = skip_whitespace(p);
   user_jiffies = (unsigned int)strtod( p, &p );
   p = skip_whitespace(p);
   nice_jiffies = (unsigned int)strtod( p, &p ); 
   p = skip_whitespace(p);
   system_jiffies = (unsigned int)strtod( p , &p ); 
   p = skip_whitespace(p);
   idle_jiffies = (unsigned int)strtod( p , &p );

   if(num_cpustates == NUM_CPUSTATES_24X)
     return user_jiffies + nice_jiffies + system_jiffies + idle_jiffies;

   p = skip_whitespace(p);
   wio_jiffies = (unsigned int)strtod( p , &p );
   p = skip_whitespace(p);
   irq_jiffies = (unsigned int)strtod( p , &p );
   p = skip_whitespace(p);
   sirq_jiffies = (unsigned int)strtod( p , &p );
  
   return user_jiffies + nice_jiffies + system_jiffies + idle_jiffies +
          wio_jiffies + irq_jiffies + sirq_jiffies; 
}   

double cpu_user_func ( void ) {
	char *p;
	double val;
	double last_user_jiffies,  user_jiffies, 
                 last_total_jiffies, total_jiffies, diff;
   
	p = update_file(&proc_stat);

	p = skip_token(p);
	user_jiffies  = strtod( p , (char **)NULL );
	total_jiffies = total_jiffies_func();

	diff = user_jiffies - last_user_jiffies; 

	if ( diff )
	val = (diff/(total_jiffies - last_total_jiffies))*100;
	else
	val = 0.0;

	last_user_jiffies  = user_jiffies;
	last_total_jiffies = total_jiffies;

	return val;
}

double cpu_nice_func ( void ) {
	char *p;
	double val;
	double last_nice_jiffies,  nice_jiffies,
			 last_total_jiffies, total_jiffies, diff;

	p = update_file(&proc_stat);
 
	p = skip_token(p);
	p = skip_token(p);
	nice_jiffies  = strtod( p , (char **)NULL );
	total_jiffies = total_jiffies_func();

	diff = (nice_jiffies  - last_nice_jiffies);

	if ( diff )
	val = (diff/(total_jiffies - last_total_jiffies))*100;
	else
	val = 0.0;

	last_nice_jiffies  = nice_jiffies;
	last_total_jiffies = total_jiffies;

	return val;
}

double cpu_system_func ( void ) {
	char *p;
	static double val;
	static double last_system_jiffies,  system_jiffies,
                 last_total_jiffies, total_jiffies, diff;
 
	p = update_file(&proc_stat);
	p = skip_token(p);
	p = skip_token(p);
	p = skip_token(p);
	system_jiffies = strtod( p , (char **)NULL );
	if (num_cpustates > NUM_CPUSTATES_24X) {
		p = skip_token(p);
		p = skip_token(p);
		p = skip_token(p);
		system_jiffies += strtod( p , (char **)NULL ); /* "intr" counted in system */
		p = skip_token(p);
		system_jiffies += strtod( p , (char **)NULL ); /* "sintr" counted in system */
	}
	total_jiffies  = total_jiffies_func();

	diff = system_jiffies  - last_system_jiffies;

	if ( diff )
	val = (diff/(total_jiffies - last_total_jiffies))*100;
	else
	val = 0.0;

	last_system_jiffies  = system_jiffies;
	last_total_jiffies = total_jiffies;   

	return val;
}
 
double cpu_idle_func ( void ) {
	char *p;
	static double val; 
	static double last_idle_jiffies,  idle_jiffies,
	              last_total_jiffies, total_jiffies, diff;
									  
	p = update_file(&proc_stat);
		      
	p = skip_token(p);
	p = skip_token(p);
	p = skip_token(p);
	p = skip_token(p);
	idle_jiffies  = strtod( p , (char **)NULL );
	total_jiffies = total_jiffies_func();
		 
	diff = idle_jiffies - last_idle_jiffies;
																							   
	if ( diff ) 
		val = (diff/(total_jiffies - last_total_jiffies))*100;
	else 
		val = 0.0; 
	last_idle_jiffies = idle_jiffies;
	last_total_jiffies = total_jiffies;
	return val; 
}

double load_one_func ( void ) {
	double val;
	val = strtod( update_file(&proc_loadavg), (char **)NULL);
	return val;
}

double load_five_func ( void ) {
	char *p;
	double val;

	p = update_file(&proc_loadavg);
	p = skip_token(p);
	val = strtod( p, (char **)NULL);

	return val;
}

double load_fifteen_func ( void ) {
	char *p;
	double val;

	p = update_file(&proc_loadavg);

	p = skip_token(p);
	p = skip_token(p);
	val = strtod( p, (char **)NULL);

	return val;
}

double mem_buffers_func ( void ) {
	char *p;
	double val;

	p = strstr( update_file(&proc_meminfo), "Buffers:" );
	if(p) {
	p = skip_token(p);
	val = atof( p ); 
	} else {
	val = 0;
	}

	return val;
}

double mem_free_func ( void ) {
	char *p;
	double val;

	p = strstr( update_file(&proc_meminfo), "MemFree:" );
	if(p) {
	p = skip_token(p);
	val = atof( p );
	} else {
	val = 0;
	}

	return val;
}

double mem_cached_func ( void ) {
   char *p;
   double val;

   p = strstr( update_file(&proc_meminfo), "Cached:");
   if(p) {
     p = skip_token(p);
     val = atof( p );
   } else {
     val = 0;
   }

   return val;
}

double swap_free_func ( void ) {
   char *p;
   double val;

   p = strstr( update_file(&proc_meminfo), "SwapFree:" );
   if(p) {
     p = skip_token(p);
     val = atof( p ); 
   } else {
     val = 0;
   }

   return val;
}

double gettimeofday_func( void ) {
	double timestamp;
	struct timeval now;
	gettimeofday(&now, NULL);
	timestamp = now.tv_sec * 1.0e6 + now.tv_usec;
	return timestamp;
}

int cpu_max_freq_func( void ) {
	char *p;
	int val;

	if ( stat(CPU_FREQ_SCALING_MAX_FREQ, &struct_stat) == 0 ) {
		if(slurpfile(CPU_FREQ_SCALING_MAX_FREQ, sys_devices_system_cpu, 32)) {
			p = sys_devices_system_cpu;
			val = (strtol( p, (char **)NULL , 10 ) / 1000 );
		}
	}
	return val;
}

int cpu_min_freq_func( void ) {
	char *p;
	int val;

	if ( stat(CPU_FREQ_SCALING_MIN_FREQ, &struct_stat) == 0 ) {
		if(slurpfile(CPU_FREQ_SCALING_MIN_FREQ, sys_devices_system_cpu, 32)) {
			p = sys_devices_system_cpu;
			val = (strtol( p, (char **)NULL , 10 ) / 1000 );
		}
	}
	return val;
}

int cpu_cur_freq_func( void ) {
	char *p;
	int val;

	if ( stat(CPU_FREQ_SCALING_CUR_FREQ, &struct_stat) == 0 ) {
		if(slurpfile(CPU_FREQ_SCALING_CUR_FREQ, sys_devices_system_cpu, 32)) {
			p = sys_devices_system_cpu;
			val = (strtol( p, (char **)NULL , 10 ) / 1000 );
		}
	}
	return val;
}

int *cpu_available_freq_func( void ) {
	char *p;
	static int val[3];

	if ( stat(CPU_FREQ_SCALING_AVAILABLE_FREQ, &struct_stat) == 0 ) {
		if(slurpfile(CPU_FREQ_SCALING_AVAILABLE_FREQ, sys_devices_system_cpu_available, 128)) {
			p = sys_devices_system_cpu_available;
			val[0] = (strtol( p, (char **)NULL , 10 ) / 1000 );
			printf (" CPU_MAX_FREQ %  :  %d \n", val[0]);
			p = skip_token(p);
			val[1] = (strtol( p, (char **)NULL , 10 ) / 1000 );
			printf (" CPU_MID_FREQ %  :  %d \n", val[1]);
			p = skip_token(p);
			val[2] = (strtol( p, (char **)NULL , 10 ) / 1000 );
			printf (" CPU_MIN_FREQ %  :  %d \n", val[2]);
		}
	}
	return val;
}

char *cpu_scaling_governor_func( void ) {
	char *p;
	if ( stat(CPU_FREQ_SCALING_GOVERNOR, &struct_stat) == 0 ) {
		if(slurpfile(CPU_FREQ_SCALING_GOVERNOR, sys_devices_system_cpu, 32)) {
			p = sys_devices_system_cpu;
		}
	}
	return p;
}

char **cpu_scaling_available_governors_func( void ) {
	static char *val[3];
	char *p;

	if ( stat(CPU_FREQ_SCALING_AVAILABLE_GOVERNORS, &struct_stat) == 0 ) {
		if(slurpfile(CPU_FREQ_SCALING_AVAILABLE_GOVERNORS, sys_devices_system_cpu_available, 128)) {
			p = sys_devices_system_cpu_available;
			printf (" CPU_AVAILABLE 1 2 3 %  :  %s \n", p);
			val[0] = strtok(p, " ");
			printf (" CPU_AVAILABLE 1 %  :  %s \n", val[0]);
			val[1] = strtok(NULL, " ");
			printf (" CPU_AVAILABLE 2 %  :  %s \n", val[1]);
			val[2] = strtok(NULL, " ");
			printf (" CPU_AVAILABLE 3 %  :  %s \n", val[2]);
		}
	}
	return val;
}

int libvirt_init(void) {
	if(virInitialize() != 0) {
		return -1;
	} else {
		return 0;
	}
}

int libvirt_config(void) {
	if(libvirt_init() == -1)
		return 0;
	virConnectPtr conn = virtConnectOpenReadOnly("xen:///");
	if(conn == NULL) {
	  printf("libvirt connection open error on uri \n"); 
	  return 0;
	}
	return 1;
}

int libvirt_read(void) {
	virConnectPtr conn = virtConnectOpenReadOnly("xen:///");
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
	if(n > 0) {
		int i;
		int *domids;

		domids = malloc(sizeof(int) * n);
		if(domids == 0) {
			printf("libvirt domain ids malloc failed");
			return -1;
		}
		
		n = virConnectListDomains(conn, domids, n);
		if(n < 0) {
			printf"Error reading list of domains \n");
		}
		
		for (i = 0; i < domids ; i++) {
			virDomainPtr dom = NULL;
			const char *name;
			char *xml = NULL;
			xmlDocPtr xml_doc = NULL;
			xmlXPathContextPtr xpath_ctx = NULL;
			xmlXPathObjectPtr xpath_obj = NULL;
			int j;

			dom = virDomainLookupByID(conn, domids[i]);
			if(dom == NULL) {
				printf("Domain no longer active or moved away .. \n");
			}
			name = virDomainGetName(dom);
			if(name == NULL) {
				printf("Domain name not valid .. \n");
			}
			xml = virDomainGetXMLDesc(dom, 0);
			xml_doc = xmlReadDoc((xmlChar *) xml, NULL, NULL, XML_PARSE_NONET);
			xpath_ctx = xmlXPathNewContext(xml_doc);
			xpath_obj = xmlXPathEval((xmlChar *) "/domain/devices/disk/target[@dev]", xpath_ctx);

			for(j = 0; j < xpath_obj->nodesetval->nodeNr; ++j) {
			
				xmlNodePtr node;
				char *path = NULL;
				node = xpath_obj->nodesetval->nodeTab[j];
				if(!node) continue;
				path = (char *) xmlGetProp (node, (xmlChar *) "dev");
				if(!path) continue;
				xmlXPathFreeObject(xpath_obj);
			}


		}


	}
}

int main(int argc, char **argv) {
	num_cpustates_func();
	printf (" CPU USER %  :  %lf \n", cpu_user_func());
	printf (" CPU NICE %  :  %lf \n", cpu_nice_func());
	printf (" CPU SYSTEM %  :  %lf \n", cpu_system_func());
	printf (" CPU IDLE %  :  %lf \n", cpu_idle_func());
	
	printf (" LOAD 1 %  :  %lf \n", load_one_func());
	printf (" LOAD 5 %  :  %lf \n", load_five_func());
	printf (" LOAD 15 %  :  %lf \n", load_fifteen_func());

	printf (" MEM FREE %  :  %lf \n", mem_free_func());
	printf (" MEM BUFF %  :  %lf \n", mem_buffers_func());
	printf (" MEM CACHE %  :  %lf \n", mem_cached_func());

	printf (" SWAP FREE %  :  %lf \n", swap_free_func());
	printf (" GET TIMESTAMP %  :  %lf \n", gettimeofday_func());

	printf (" CPU_MAX_FREQ %  :  %d \n", cpu_max_freq_func());
	printf (" CPU_MIN_FREQ %  :  %d \n", cpu_min_freq_func());
	printf (" CPU_CUR_FREQ %  :  %d \n", cpu_cur_freq_func());
	int *val;
	val = cpu_available_freq_func();
	printf (" CPU_AVAILABLE_FREQ %  :  %d , %d , %d \n", val[0], val[1], val[2]);

	printf (" CPU_FREQ_GOVERNOR %  :  %s \n", cpu_scaling_governor_func());
	char **avail;
	avail = cpu_scaling_available_governors_func();
	printf (" CPU_FREQ_AVAILABLE_GOVERNORS %  :  %s , %s , %s \n", avail[0], avail[1], avail[2]);
}
