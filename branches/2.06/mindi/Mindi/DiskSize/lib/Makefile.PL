use ExtUtils::MakeMaker;
$Verbose = 1;
WriteMakefile(
    NAME      => 'Mindi::DiskSize::lib',
    SKIP      => [qw(all static static_lib dynamic dynamic_lib)],
    clean     => {'FILES' => 'libdisksize.a'},
);

sub MY::top_targets {
        '
all :: static

static ::       libdisksize.a

libdisksize.a: $(O_FILES)
	$(AR) cr libdisksize.a $(O_FILES)
	$(RANLIB) libdisksize.a
';
}
