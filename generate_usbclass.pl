#!/usr/bin/perl

print q(/* This is auto-generated from </usr/share/usb.ids>, don't modify! */

struct {
  unsigned long id;
  const char *name;
} usbclasses[] = {
);

while (<>) {
    chomp;
    if (/^C\s+([\da-f]+)\s+(.*)/) {  #- I want alphanumeric but I can't get this [:alnum:] to work :-(
	($cat, $cat_descr) = ($1, $2);
    } elsif (/^\t([\da-f]+)\s+(.*)/ && defined $cat) {
	($sub, $sub_descr) = ($1, $2);
	$sub =~ /^[\da-f]{2}$/ or die "bad line $.: sub category number badly formatted ($_)\n";
	push @without_prot, [ "0x$cat$sub"."00", "/* $. */  { 0x$cat$sub".qq(00, "$cat_descr|$sub_descr" },\n) ];
    } elsif (/^\t\t([\da-f]+)\s+(.*)/ && defined $cat) {
	push @everything, qq(/* $. */  { 0x$cat$sub$1, "$cat_descr|$sub_descr|$2" },\n);
    } elsif (/^\S/) {
      undef $cat;
    }
}

foreach $l (@without_prot) {
    grep { /{ $l->[0], / } @everything or push @everything, $l->[1];
}

print sort @everything;

print '
};

int nb_usbclasses = sizeof(usbclasses) / sizeof(*usbclasses);

extern const char *usb_class2text(unsigned long class_) {
  int i;
  for (i = 0; i < nb_usbclasses; i++)
    if (usbclasses[i].id == class_) return usbclasses[i].name;

  return "";
}

';
