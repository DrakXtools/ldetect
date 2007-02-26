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
use detect_devices;

my %bus_get_description = (
    usb => sub {
        my ($dev_path, $_values) = @_;
        my $full_path = dirname($dev_path) . "/" . readlink($dev_path);
        my $parent_path = dirname($full_path);
        chomp_(cat_("$parent_path/product"));
    },
);

sub list {
    my (@bus) = @_;
    @bus = qw(usb) if !@bus;
    map {
        my ($modalias_path, $desc);
        if (ref $_) {
            my $device = $_;
            $modalias_path = sprintf('/sys/bus/pci/devices/%04x:%02x:%02x.%x/modalias', $device->{pci_domain}, $device->{pci_bus}, $device->{pci_device}, $device->{pci_function});
            $desc = $device->{description};
        } else {
            $modalias_path = $_;
        }
        my $modalias = chomp_(cat_($modalias_path));
        my $dev_path = dirname($modalias_path);
        my ($bus, $values) = $modalias =~ /^([^:]+):(\S+)$/; # modalias::get_bus
        my @modules = modalias::get_modules($modalias);
        my $module = first(@modules) || "unknown";
        my $modules = @modules > 1 ? " (" . join(",", @modules) . ")" : "";
        if (my $get_desc = $bus_get_description{$bus} and !$desc) {
            $desc = $get_desc->($dev_path, $values);
        }
        $desc ||= "unknown";
        { module => $module, descr => $desc, modules => $modules };
    } detect_devices::pci_probe(), map { glob("/sys/bus/$_/devices/*/modalias") } @bus;
}

1;
