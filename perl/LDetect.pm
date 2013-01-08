package LDetect;

use strict;
use warnings;

use XSLoader;

our @ISA = qw(); # help perl_checker
our $VERSION = '0.13.1';
# perl_checker: EXPORT-ALL

XSLoader::load('LDetect', $VERSION);

1;
