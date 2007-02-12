package MDV::Lspciusb;
#*****************************************************************************
#
#  Copyright (c) 2006-2007 Mandriva SA
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2, as
#  published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#*****************************************************************************
#
# $Id$

use lib qw(/usr/lib/libDrakX);
use modalias;
use MDK::Common;

sub read_pciids {
    my ($f) = @_;
    my %drivers;
    my ($id1, $id2, $vendor, $line);
    foreach (cat_($f)) {
	chomp; $line++;
	next if /^#/ || /^;/ || /^\s*$/;
	if (/^C\s/) {
	    last;
	} elsif (my ($subid1, $subid2, $text) = /^\t\t(\S+)\s+(\S+)\s+(.+)/) {
	    $text =~ s/\t/ /g;
	    $id1 && $id2 or die "$f:$line: unexpected device\n";
	    $drivers{sprintf qq(%04x%04x%04x%04x), hex($id1), hex($id2), hex($subid1), hex($subid2)} = "$vendor|$text";
	} elsif (/^\t(\S+)\s+(.+)/) {
	    ($id2, $text) = ($1, $2);
	    $text =~ s/\t/ /g;
	    $id1 && $id2 or die "$f:$line: unexpected device\n";
	    $drivers{sprintf qq(%04x%04xffffffff), hex($id1), hex($id2)} = "$vendor|$text";
	} elsif (/^(\S+)\s+(.+)/) {
	    $id1 = $1;
	    $vendor = $2;
	} else {
	    warn "$f:$line: bad line: $_\n";
	}
    }
    \%drivers;
}

my $pciids = read_pciids("/usr/share/pci.ids");

my %bus_get_description = (
    pci => sub {
        my ($dev_path, $values) = @_;
        if (my @ids = $values =~ /^v([[:xdigit:]]{8})d([[:xdigit:]]{8})sv([[:xdigit:]]{8})sd([[:xdigit:]]{8})/) {
            my ($v, $d, $sv, $sd) = map { hex ($_) } @ids;
            return $pciids->{sprintf(qq(%04x%04x%04x%04x), $v, $d, $sv, $sd)} ||
              $pciids->{sprintf(qq(%04x%04xffffffff), $v, $d)};
        }
    },
    usb => sub {
        my ($dev_path, $values) = @_;
        $full_path = dirname($dev_path) . "/" . readlink($dev_path);
        $parent_path = dirname($full_path);
        chomp_(cat_("$parent_path/product"));
    },
);

sub list() {
    map {
        my $modalias_path = $_;
        my $modalias = chomp_(cat_($modalias_path));
        my $dev_path = dirname($modalias_path);
        my ($bus, $values) = $modalias =~ /^([^:]+):(\S+)$/; # modalias::get_bus
        my @modules = modalias::get_modules($modalias);
        my $module = first(@modules) || "unknown";
        my $modules = @modules > 1 ? " (" . join(",", @modules) . ")" : "";
        my $desc;
        if (my $get_desc = $bus_get_description{$bus}) {
            $desc = $get_desc->($dev_path, $values);
        }
        $desc ||= "unknown";
        { module => $module, descr => $desc, modules => $modules }
    } map { glob("/sys/bus/$_/devices/*/modalias") } qw(pci usb);
}

1;
