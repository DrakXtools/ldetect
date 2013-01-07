use ExtUtils::MakeMaker;

$CC = 'g++';
$libs = '-L.. -lldetect';

WriteMakefile(
    NAME		=> 'ldetect',
    VERSION_FROM	=> 'ldetect.pm',
    OPTIMIZE		=> '-Os',
    OBJECT		=> "ldetect.o ",
    CC			=> $CC,
    LD			=> $CC,
    XSOPT		=> '-C++',
    INC			=> '-I..',
    LIBS		=> [$libs],
    DEFINE		=> '-std=gnu++11',
);
