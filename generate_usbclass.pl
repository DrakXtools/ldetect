#!/usr/bin/perl

print q(/* This is auto-generated from </usr/share/usb.ids>, don't modify! */

#include <string>
#include "usb.h"

namespace ldetect {

struct node {
  uint32_t id;
  // We'll always be doing a copy of this on use, so don't bother using std::string...
  const char *name;
  uint32_t nb_subnodes;
  const struct node *subnodes;
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

    print "${prefix}static const struct node $name\[] = {\n";
    foreach (@l) {
	my $nb = @{$_->[2]};
	my $sub = $nb ? $name . '_' . $_->[0] : 'nullptr';
	printf qq($prefix  { 0x%x, "%s", $nb, $sub },\n), hex($_->[0]), $_->[1];
    }
    print "$prefix};\n";
}

dump_it(\@l, "classes", '');

print '

static const uint32_t nb_classes = sizeof(classes) / sizeof(*classes);

static void lookup(usb_class_text &p, uint32_t *a_class, int kind, uint32_t nb_nodes, const struct node *nodes) {
    for (uint32_t i = 0; i < nb_nodes; i++)
	if (nodes[i].id == a_class[kind]) {
	    if (kind == 0)
		p.class_text = nodes[i].name;
	    else if (kind == 1)
		p.sub_text = nodes[i].name;
	    else if (kind == 2)
		p.prot_text = nodes[i].name;

	    lookup(p, a_class, ++kind, nodes[i].nb_subnodes, nodes[i].subnodes);
	    return;
	}
}

struct usb_class_text usb_class2text(uint32_t class_id) {
    uint32_t a_class[3] = { (class_id >> 16) & 0xff, (class_id >> 8) & 0xff, class_id & 0xff };
    usb_class_text p;
    if (a_class[0] != 0xff) lookup(p, a_class, 0, nb_classes, classes);
	return p;
}

}
';
