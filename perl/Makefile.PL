use ExtUtils::MakeMaker;

$CC = 'g++';
$libs = '-L.. -lldetect';

WriteMakefile(
    NAME		=> 'LDetect',
    VERSION_FROM	=> 'LDetect.pm',
    OPTIMIZE		=> '-Os',
    OBJECT		=> "LDetect.o ",
    CC			=> $CC,
    LD			=> $CC,
    XSOPT		=> '-C++',
    INC			=> '-I..',
    LIBS		=> [$libs],
    DEFINE		=> '-std=gnu++11',
);
