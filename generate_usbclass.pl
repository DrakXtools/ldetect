#!/usr/bin/perl

print q(/* This is auto-generated from </usr/share/usb.ids>, don't modify! */

#include <stdlib.h>
#include "libldetect.h"

struct node {
  int id;
  const char *name;
  int nb_subnodes;
  struct node *subnodes;
};

);

my (@l, $sub, $subsub);
while (<>) {
    chomp;
    if (/^C\s+([\da-f]+)\s+(.*)/) {  #- I want alphanumeric but I can't get this [:alnum:] to work :-(
	push @l, [ $1, $2, $sub = [] ];
	undef $subsub;
    } elsif (/^\t([\da-f]+)\s+(.*)/ && defined $sub) {
	my ($sub_id, $sub_descr) = ($1, $2);
	$sub_id =~ /^[\da-f]{2}$/ or die "bad line $.: sub category number badly formatted ($_)\n";
	push @$sub, [ $sub_id, $sub_descr, $subsub = [] ];
    } elsif (/^\t\t([\da-f]+)\s+(.*)/ && defined $subsub) {
	push @$subsub, [ $1, $2, [] ];
    } elsif (/^\S/) {
	undef $sub;
	undef $subsub;
    }
}

sub dump_it {
    my ($l, $name, $prefix) = @_;

    my @l = sort { $a->[0] cmp $b->[0] } @$l or return;

    dump_it($_->[2], $name . '_' . $_->[0], "$prefix  ") foreach @l;

    print "${prefix}static struct node $name\[] = {\n";
    foreach (@l) {
	my $nb = @{$_->[2]};
	my $sub = $nb ? $name . '_' . $_->[0] : 'NULL';
	printf qq($prefix  { 0x%x, "%s", $nb, $sub },\n), hex($_->[0]), $_->[1];
    }
    print "$prefix};\n";
}

dump_it(\@l, "classes", '');

print '

static int nb_classes = sizeof(classes) / sizeof(*classes);

static void lookup(const char **p, int *a_class, int kind, int nb_nodes, struct node *nodes) {
  int i;
  for (i = 0; i < nb_nodes; i++)
    if (nodes[i].id == a_class[kind]) {
      p[kind] = nodes[i].name;
      return lookup(p, a_class, kind + 1, nodes[i].nb_subnodes, nodes[i].subnodes);
    }
}

struct usb_class_text usb_class2text(unsigned long class_id) {
  const char *p[3] = { NULL, NULL, NULL };
  int a_class[3] = { (class_id >> 16) & 0xff, (class_id >> 8) & 0xff, class_id & 0xff };
  if (a_class[0] != 0xff) lookup(p, a_class, 0, nb_classes, classes);
  {
    struct usb_class_text r = { p[0], p[1], p[2] };
    return r;
  }
}

';
