#!/usr/bin/perl

print q(/* This auto-generated from <usb.h>, don't modify! */

struct {
  unsigned short id;
  const char *name;
} usbclasses[] = {
);

while (<>) {
    chomp;
    if (/^C\s+(\d+)\s+(.*)/) {
	($cat, $descr) = ($1, $2);
    } elsif (/^\t\t(\d+)\s+(.*)/ && defined $cat) {
	print qq(/* $. */  { 0x$cat$1, "$descr|$2" },\n);
    } elsif (/^\S/) {
      undef $cat;
    }
}

print '
};

int nb_usbclasses = sizeof(usbclasses) / sizeof(*usbclasses);

extern const char *usb_class2text(unsigned short class) {
  int i;
  for (i = 0; i < nb_usbclasses; i++)
    if (usbclasses[i].id == class) return usbclasses[i].name;

  return "";
}

';
