#!/usr/bin/perl -w
#
# Creates changelog for packages from Changelog files in the apps
#
# $Id$
#
# Syntax : mkchangelog dtype package-name output-file
#

use strict;
use Date::Manip;
use File::Basename;
use English;

my $log = "";
my $dtype = $ARGV[0];
my $pkg = $ARGV[1];
my $pkg2;
my $outfile = $ARGV[2];
my $chglog = "";
my $ndate = "";
my $n2date = "";
my $tmp = "";
my $ver = "";
my $date = "";

# For date handling
$ENV{LANG}="C";

die "Syntax : mkchangelog dtype package-name output-file" 
	if ((not (defined $dtype)) || ($dtype eq "") || 
		(not (defined $pkg)) || ($pkg eq "") || 
		(not (defined $outfile)) || ($outfile eq ""));

my $TOOLHOME;
$tmp = dirname($PROGRAM_NAME);
if ($tmp =~ /^\//) {
	$TOOLHOME = $tmp;
	}
else {
	$TOOLHOME = "$ENV{PWD}/$tmp";
	}

die "TOOLHOME doesn't exist" if (not (defined $TOOLHOME));

if (-f "$TOOLHOME/../$pkg/ChangeLog") {
	$chglog = "$TOOLHOME/../$pkg/ChangeLog";
	}
else {
	$pkg2 = $pkg;
	$pkg2 =~ s/-..*//;
	if (-f "$TOOLHOME/../$pkg2/ChangeLog") {
		$chglog = "$TOOLHOME/../$pkg2/ChangeLog";
		}
	else {
		die "Unable to find a ChangeLog file for $pkg\n";
	}
}
print "Using $chglog as input ChangeLog file for $pkg\n";

open(INPUT,"$chglog") || die "Unable to open $chglog (read)";
open(OUTPUT,"> $outfile") || die "Unable to open $outfile (write)";
# Skip first 4 lines
$tmp = <INPUT>;
$tmp = <INPUT>;
$tmp = <INPUT>;
if ($dtype eq "announce") {
	print OUTPUT $tmp;
}
$tmp = <INPUT>;
if ($dtype eq "announce") {
	print OUTPUT $tmp;
}

# Handle each block separated by newline
while (<INPUT>) {
	($ver, $date) = split(/ /);
	chomp($date);
	$date =~ s/\(([0-9-]+)\)/$1/;
	#print "**$date**\n";
	$ndate = UnixDate($date,"%a", "%b", "%d", "%Y");
	$n2date = &UnixDate($date,"%a, %d %b %Y %H:%M:%S %z");
	#print "**$ndate**\n";
	if ($dtype eq "rpm") {
		print OUTPUT "* $ndate Bruno Cornec <bruno\@mondorescue.org> $ver\n";
		print OUTPUT "- Updated to $ver\n";
		}
	if ($dtype eq "deb") {
		print OUTPUT "$pkg ($ver) unstable; urgency=low\n";
		print OUTPUT "\n";
		}

	$tmp = <INPUT>;	
	while ($tmp !~ /^$/) {
		print OUTPUT "  * $tmp";
		last if (eof(INPUT));
		$tmp = <INPUT>;
	}
	print OUTPUT "\n";

	if ($dtype eq "deb") {
		print OUTPUT " -- Bruno Cornec <bruno\@mondorescue.org>  $n2date\n\n";
		print OUTPUT "\n";
		}

	last if (eof(INPUT));
	last if ($dtype eq "announce");
}
close(OUTPUT);
close(INPUT);
