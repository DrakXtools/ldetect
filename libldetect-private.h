extern int pciusb_find_modules(struct pciusb_entries *entries, const char *fpciusbtable, int no_subid);
extern void pciusb_initialize(struct pciusb_entry *e);

#define MAX_DEVICES 100
#define BUF_SIZE 512


extern char *proc_usb_path;
