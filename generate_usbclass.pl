#!/usr/bin/perl

print q(/* This auto-generated from <usb.h>, don't modify! */

struct {
  unsigned long id;
  const char *name;
} usbclasses[] = {
);

while (<>) {
    chomp;
    if (/^C\s+(\d+)\s+(.*)/) {
	($cat, $cat_descr) = ($1, $2);
    } elsif (/^\t(\d+)\s+(.*)/ && defined $cat) {
	($sub, $sub_descr) = ($1, $2);
	$sub =~ /^\d\d$/ or die "bad line $.: sub category number badly formatted ($_)\n";
    } elsif (/^\t\t(\d+)\s+(.*)/ && defined $cat) {
	print qq(/* $. */  { 0x$cat$sub$1, "$cat_descr|$sub_descr|$2" },\n);
    } elsif (/^\S/) {
      undef $cat;
    }
}

print '
};

int nb_usbclasses = sizeof(usbclasses) / sizeof(*usbclasses);

extern const char *usb_class2text(unsigned long class) {
  int i;
  for (i = 0; i < nb_usbclasses; i++)
    if (usbclasses[i].id == class) return usbclasses[i].name;

  return "";
}

';
