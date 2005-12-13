#!/usr/bin/perl -w
#
# $Id$
#
# parted2fdisk: fdisk like interface for parted 
# [developped for mindi/mondo http://mondorescue.berlios.de]
#
# Aims to be architecture independant (i386/ia64)
# Tested on RHAS 2.1 ia64 - Mandrake 9.0 ia64 - RHAS 3.0 ia64
#
# (c) Bruno Cornec <Bruno.Cornec@hp.com>
# Licensed under the GPL

use strict;

$ENV{LANG} = "C";
$ENV{LANGUAGE} = "C";
$ENV{LC_ALL} = "C";

# Log 
my $flog = "/tmp/parted2fdisk.log";
open(FLOG, "> $flog") || die "Unable to open $flog";

my $fdisk = "/sbin/fdisk";
my $parted = "/sbin/parted";

my $i;
my $l;
my $part;
my $wpart;
my $start = "";
my $end = "";
my $cylstart;
my $cylend;
my %start;
my %end;
my %type;
my %flags;
my $arch;
my $fake = 0;

# Determine on which arch we're running
if (defined ($ENV{ARCH})) {
	$arch = $ENV{ARCH};
} else {
	$arch = `"/bin/arch"`;
	chomp($arch);
}

#
# Looking for fdisk
#
$fdisk = is_lsb($fdisk);
#
# We always use fdisk except on ia64 with GPT types of 
# partition tables where we need parted
# All should return fdisk like format so that callers
# think they have called fdisk directly
#
my $un;
my $type;
my $args = "";
my $device = "";
my $endmax = "";

if ($#ARGV < 0) {
	printf FLOG "No arguments given exiting ...\n";
	mysyn();
}

my %pid = (	"FAT" => "6",
		"fat32" => "b",
		"fat16" => "e",
		"ext2" => "83",
		"ext3" => "83",
		"linux-swap" => "82",
		"LVM" => "8e",
		"" => "",
	);
my %pnum;

# Reverse table of pid
while (($i,$l) = each %pid) {
	next if ($i eq "ext2");
	$pnum{$l} = $i;
}

foreach $i (@ARGV) {
	# We support at most one option and one device
	print FLOG "Parameter found : $i\n";
	if ($i =~ /^\/dev\//) {
		$device = $i;
		next;
	} elsif ($i =~ /^-/) {
		$args = $i;
		next;
	} else {
		mysyn();
	}
}

if (($args ne "") and ($device eq "")) {
	mysyn();
}

# -s takes a partition as arg
if ($args =~ /-s/) {
	$wpart = $device;
	$device =~ s/[0-9]+$//;
}

if ($args =~ /-n/) {
	print FLOG "Fake mode. Nothing will be really done\n";
	$fake = 1;
}

print FLOG "Called with device $device and arg $args\n";

if ($arch =~ /^ia64/) {
	# Check partition table type
	print FLOG "We're on ia64 ...\n";
	$parted = is_lsb($parted);
	$type = which_type($device);
	if ($type ne "msdos") {
		print FLOG "Not an msdos type of disk label\n";
		if ($args =~ /-l/) {
			fdisk_list($device,undef,\%start,\%end, 1);
		} elsif ($args =~ /-s/) {
			fdisk_list($device,$wpart,\%start,\%end, 1);
		} elsif (($args =~ /-/) and ($fake == 0)) {
			printf FLOG "Option not supported ($args) ...\n";
			printf FLOG "Please report to the author\n";
			mysyn();
		} else {
			# Read fdisk orders on stdin and pass them to parted
			# on the command line as parted doesn't read on stdin
			print FLOG "Translating fdisk command to parted\n";
			while ($i = <STDIN>) {
				if ($i =~ /^p$/) {
					fdisk_list($device,undef,\%start,\%end, 1);
				}
				elsif ($i =~ /^n$/) {
					fdisk_list($device,undef,\%start,\%end, 0);
					if ($type ne "gpt") {
						print FLOG "Forcing GPT type of disk label\n";
						print FLOG "mklabel gpt\n";
						system "$parted -s $device mklabel gpt\n" if ($fake == 0);
						$type = "gpt";
					}
					$l = <STDIN>;
					if (not (defined $l)) {
						print FLOG "no primary/extended arg given for creation... assuming primary\n";
						$l = "p";
					}
					chomp($l);
					$part = <STDIN>;
					if ((not (defined $part)) || ($part eq "")) {
						print FLOG "no partition given for creation... skipping\n";
						next;
					}
					chomp($part);
					$cylstart = <STDIN>;
					chomp($cylstart);
					if ((not (defined $cylstart)) || ($cylstart eq "")) {
						if (defined $start{$part-1}) {
							# in MB => cyl
							$cylstart = sprintf("%d",$end{$part-1}*1048576/$un + 1);
							print FLOG "no start cyl given for creation... assuming the following $cylstart\n";
						} else {
							print FLOG "no start cyl given for creation... assuming the following 1\n";
							$cylstart = 1;
						}
					}
					$cylstart = 1 if ($cylstart < 1);
					print FLOG "start cyl : $cylstart\n";
					$un = get_un($device, "", 0);
					# parted needs MB
					if ($cylstart == 1) {
						$start = 0.01;
					} else {
						$start = $cylstart* $un / 1048576 +0.001;
					}
					# this is a size in B/KB/MB/GB

					$endmax = get_max($device);
					$cylend = <STDIN>;
					chomp($cylend);
					if ((not (defined $cylend)) || ($cylend eq "")) {
						print FLOG "no end cyl given for creation... assuming full disk)\n";
						$cylend = $endmax;
					}
					# Handles end syntaxes (+, K, M, ...)
					# to give cylinders
					if ($cylend =~ /^\+/) {
						$cylend =~ s/^\+//;
						if ($cylend =~ /K$/) {
							$cylend =~ s/K$//;
							$cylend *= 1024;
						} elsif ($cylend =~ /M$/) {
							$cylend =~ s/M$//;
							$cylend *= 1048576;
						} elsif ($cylend =~ /G$/) {
							$cylend =~ s/G$//;
							$cylend *= 1073741824;
						}
						# This gives the number of cyl
						$cylend /= $un;
						$cylend = sprintf("%d",$cylend);
						$cylend += $cylstart - 0.001;
						# We now have the end cyl
					}
					$cylend = $endmax if ($cylend > $endmax); 
					print FLOG "end cyl : $cylend\n";
					# parted needs MB
					$end = $cylend * $un / 1048576;
					print FLOG "n $l $part $cylstart $cylend => mkpart primary $start $end\n";
					system "$parted -s $device mkpart primary ext2 $start $end\n" if ($fake == 0);
				}
				elsif ($i =~ /^d$/) {
					$part = <STDIN>;
					if (not (defined $part)) {
						print FLOG "no partition given for deletion... skipping\n";
						next;
					}
					chomp($part);
					print FLOG "d $part => rm $part\n";
					system "$parted -s $device rm $part\n" if ($fake == 0);
					get_parted($device,undef,\%start,\%end,undef);
				}
				elsif ($i =~ /^w$/) {
					print FLOG "w => quit\n";
				}
				elsif ($i =~ /^t$/) {
					$part = <STDIN>;
					if (not (defined $part)) {
						print FLOG "no partition given for tagging... skipping\n";
						next;
					}
					chomp($part);
					$l = <STDIN>;
					if (not (defined $l)) {
						print FLOG "no type given for tagging partition $part... skipping\n";
						next;
					}
					chomp($l);
					if (not (defined $pnum{$l})) {
						print FLOG "no partition number given for $l... please report to the author\n";
						next;
					}
					print FLOG "t $part $l => mkfs $part $pnum{$l}\n";
					system "$parted -s $device mkfs $part $pnum{$l}\n" if ($fake == 0);
				}
				elsif ($i =~ /^a$/) {
					$part = <STDIN>;
					if (not (defined $part)) {
						print FLOG "no partition given for tagging... skipping\n";
						next;
					}
					chomp($part);
					print FLOG "a $part => set $part boot on\n";
					system "$parted -s $device set $part boot on\n" if ($fake == 0);
				}
				elsif ($i =~ /^q$/) {
					print FLOG "q => quit\n";
				}
				else {
					print FLOG "Unknown command: $i\n";
					next;
				}
					
			}
		}
		myexit(0);
	}
}

#
# Else everything is for fdisk
#
# Print only mode
print FLOG "Passing everything to the real fdisk\n";
my $fargs = join(@ARGV);

if ($args =~ /^-/) {
	# -l or -s
	open (FDISK, "$fdisk $fargs |") || die "Unable to read from $fdisk";
	while (<FDISK>) {
		print;
	}
	close(FDISK);
} else {
	# Modification mode
	open (FDISK, "| $fdisk $fargs") || die "Unable to modify through $fdisk";
	while (<STDIN>) {
		print FDISK;
	}
	close(FDISK);
	close(STDIN);
}
myexit(0);


# Is your system LSB ?
sub is_lsb {

my $cmd = shift;
my $basename = basename($cmd);

if (not (-x $cmd)) {
	print FLOG "Your system is not LSB/mondo compliant: $basename was not found as $cmd\n";
	print FLOG "Searching elswhere...";
	foreach $i (split(':',$ENV{PATH})) {
		if (-x "$i/$basename") {
			$cmd = "$i/$basename";
			print FLOG "Found $cmd, using it !\n";
			last;
		}
	}
	if (not (-x $cmd)) {
		print FLOG "Your system doesn't provide $basename in the PATH\n";
		print FLOG "Please correct it before relaunching\n";
		myexit(-1);
	}
}
return($cmd);
}

sub fdisk_list {

my $device = shift;
my $wpart = shift;
my $start = shift;
my $end = shift;
my $verbose = shift;

my $un;
my $endmax;
my $d;
my $n;

my %cmt = (	"FAT" => "FAT",
		"ext2" => "Linux",
		"ext3" => "Linux",
		"linux-swap" => "Linux swap",
		"LVM" => "Linux LVM",
		"fat16" => "fat16",
		"fat32" => "fat32",
		"" => "Linux",
);

my $part;
my $mstart;
my $mend;
my $length;
my $pid;
my $cmt;
format FLOG1 =
@<<<<<<<<<<<< @>>>>>>>> @>>>>>>>> @>>>>>>>> @>>>  @<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$part,        $mstart,   $mend,   $length,  $pid, $cmt
.
format FLOG2 =
@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$part,
              @>>>>>>>> @>>>>>>>> @>>>>>>>> @>>>  @<<<<<<<<<<<<<<<<<<<<<<<<<<<<
              $mstart,   $mend,   $length,  $pid, $cmt
.
format STDOUT1 =
@<<<<<<<<<<<< @>>>>>>>> @>>>>>>>> @>>>>>>>> @>>>  @<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$part,        $mstart,   $mend,   $length,  $pid, $cmt
.
format STDOUT2 =
@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$part,
              @>>>>>>>> @>>>>>>>> @>>>>>>>> @>>>  @<<<<<<<<<<<<<<<<<<<<<<<<<<<<
              $mstart,   $mend,   $length,  $pid, $cmt
.
#/dev/hda1             1     77579  39099374+  ee  EFI GPT

#
# Keep Fdisk headers
#
# this will return bytes
$un = get_un ($device,$wpart,$verbose);

$endmax = get_max($device);

# This will return MB
get_parted ($device,$start,$end,\%type,\%flags);

while (($n,$d) = each %type) {
	# Print infos fdisk like
	$part = ${device}.$n;
	# start and end are in cylinder in fdisk format
	# so return in MB * 1MB / what represents 1 cyl in B
	$mstart = sprintf("%d",$$start{$n}*1048576/$un);
	$mstart = 1 if ($mstart < 1);
	$mend = sprintf("%d",$$end{$n}*1048576/$un - 1);
	$mend = $endmax if ($mend > $endmax);
	# length is in 1K blocks
	$length = sprintf("%d",($mend-$mstart+1)*$un/1024);
	$pid = $pid{$type{$n}};
	$cmt = $cmt{$type{$n}};
	#print FLOG "$part - $mstart - $mend - $length\n";

	if ($verbose == 1) {
		if (not (defined $wpart)) {
			if (length($part) > 13) {
				open(STDOUT2,">&STDOUT") || die "Unable to open STDOUT2";
				select(STDOUT2);
				write;
				open(FLOG2,">&FLOG") || die "Unable to open FLOG2";
				select(FLOG2);
				write;
				select(STDOUT);
				close(FLOG2);
				close(STDOUT2);
			} else {
				open(STDOUT1,">&STDOUT") || die "Unable to open STDOUT1";
				select(STDOUT1);
				write;
				open(FLOG1,">&FLOG") || die "Unable to open FLOG1";
				select(FLOG1);
				write;
				select(STDOUT);
				close(FLOG1);
				close(STDOUT1);
			}
		} else {
			# manage the -s option of fdisk here
			print "$length\n" if ($part eq $wpart);
			print FLOG "$part has $length KBytes\n" if ($part eq $wpart);
		}
	}
}
close(FDISK);
close(PARTED);
}

# 
# Get max size from fdisk
#
sub get_max {

my $device = shift;
my $max = 0;
my $foo;

open (FDISK, "$fdisk -l $device |") || die "Unable to read from $fdisk";
while (<FDISK>) {
	if ($_ =~ /heads/) {
		chomp;
		$max = $_;
		$max =~ s/.* ([0-9]+) cylinders/$1/;
	}
}
close(FDISK);
print FLOG "get_max returns $max\n";
return($max);
}

# 
# Get units from fdisk (cylinder size)
#
sub get_un {

my $device = shift;
my $wpart = shift;
my $verbose = shift;
my $un = 0;
my $foo;

open (FDISK, "$fdisk -l $device |") || die "Unable to read from $fdisk";
while (<FDISK>) {
	print if (($_ !~ /^\/dev\//) and (not (defined $wpart)) and ($verbose == 1));
	if ($_ =~ /^Units/) {
		($foo, $un , $foo) = split /=/;
		$un =~ s/[A-z\s=]//g;
		$un = eval($un);
	}
}
close(FDISK);
print FLOG "get_un returns $un\n";
return($un);
}

# 
# Parted gives info in MB
#
sub get_parted {

my $device = shift;
my $start = shift;
my $end = shift;
my $type = shift;
my $flags = shift;
my $d;
my $n;

open (PARTED, "$parted -s $device print |") || die "Unable to read from $parted";
# Skip 3 first lines
$d = <PARTED>;
$d = <PARTED>;
$d = <PARTED>;
print FLOG "Got from parted: \n";
print FLOG "Minor    Start       End     Filesystem                        Flags\n";
# Get info from each partition line
while (($n,$d) = split(/\s/, <PARTED>,2)) {
	chomp($d);
	next if ($n !~ /^[1-9]/);
	$d =~ s/^\s*//;
	$d =~ s/\s+/ /g;
	($$start{$n},$$end{$n},$$type{$n},$$flags{$n}) = split(/ /,$d);
	$$start{$n} = "" if (not defined $$start{$n});
	$$end{$n} = "" if (not defined $$end{$n});
	$$type{$n} = "" if (not defined $$type{$n});
	$$flags{$n} = "" if (not defined $$flags{$n});
	print FLOG "$n      $$start{$n}      $$end{$n}     $$type{$n}  $$flags{$n}\n";
}
close(PARTED);
}

# Based on Version 2.4  27-Sep-1996  Charles Bailey  bailey@genetics.upenn.edu
# in Basename.pm

sub basename {

my($fullname) = shift;

my($dirpath,$basename);

($dirpath,$basename) = ($fullname =~ m#^(.*/)?(.*)#s);

return($basename);
}

sub myexit {

my $val=shift;

close(FLOG);
exit($val);
}

sub which_type {

my $device = shift;
my $type = "";

open (FDISK, "$fdisk -l $device |") || die "Unable to read from $fdisk";
while (<FDISK>) {
	if ($_ =~ /EFI GPT/) {
		$type= "gpt";
		print FLOG "Found a GPT partition format\n";
		last;
	}
}
close(FDISK);
open (PARTED, "$parted -s $device print|") || die "Unable to read from $fdisk";
while (<PARTED>) {
	if ($_ =~ /Disk label type: msdos/) {
		$type= "msdos";
		print FLOG "Found a msdos partition format\n";
		last;
	}
}
close(FDISK);
return ($type);
}

sub mysyn {
	print "Syntax: $0 [-l] device | [-s] partition\n";
	myexit(-1);
}
