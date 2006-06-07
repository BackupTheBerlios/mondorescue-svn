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

my $log = "";
my $dtype = $ARGV[0];
my $pkg = $ARGV[1];
my $pkg2;
my $outfile = $ARGV[2];
my $TOOLHOME = $ENV{TOOLHOME};
my $chglog = "";
my $ndate = "";
my $tmp = "";
my $ver = "";
my $date = "";

# For date handling
$ENV{LANG}="C";

die "Syntax : mkchangelog dtype package-name output-file" 
	if ((not (defined $dtype)) || ($dtype eq "") || 
		(not (defined $pkg)) || ($pkg eq "") || 
		(not (defined $outfile)) || ($outfile eq ""));

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
$tmp = <INPUT>;

# Handle each block separated by newline
while (<INPUT>) {
	($ver, $date) = split(/ /);
	chomp($date);
	$date =~ s/\(([0-9-]+)\)/$1/;
	#print "**$date**\n";
	$ndate = UnixDate($date,"%a", "%b", "%d", "%Y");
	#print "**$ndate**\n";
	if ($dtype eq "rpm") {
		printf OUTPUT "* $ndate Bruno Cornec <bruno\@mondorescue.org> $ver\n";
		printf OUTPUT "- Updated to $ver\n";

		$tmp = <INPUT>;	
		while ($tmp !~ /^$/) {
			printf OUTPUT $tmp;
			$tmp = <INPUT>;	
		}
		printf OUTPUT "\n";
	}
}
close(OUTPUT);
close(INPUT);
