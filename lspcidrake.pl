#!/usr/bin/perl
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
use MDV::Lspciusb;

if (@ARGV) {
    print "$_: " . join(",", modalias::get_modules($_)) . "\n" foreach @ARGV;
    exit();
}

foreach my $device (MDV::Lspciusb::list()) {
    print "$device->{module}\t: $device->{descr}$device->{modules}\n";
}
