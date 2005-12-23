# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl test.pl'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use Test;
BEGIN { plan tests => 1 };
use Mindi::DiskSize qw(:all);
ok(1); # If we made it this far, we're ok.

#########################

# Insert your test code below, the Test module is use()ed here so read
# its man page ( perldoc Test ) for help writing this test script.
my $disk_size_k = get_size_of_disk ("/dev/ad2");
my $disk_siz2_k = get_size_of_disk ("/dev/ad2a");
print "sizeof /dev/ad2  = $disk_size_k\n";
print "sizeof /dev/ad2a = $disk_siz2_k\n";
skip ($>,  $disk_size_k);
skip ($>, $disk_siz2_k);
