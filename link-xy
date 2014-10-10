#!/usr/bin/perl

use strict;
use warnings;

use SpiNN::Cmd;
use SpiNN::Struct;

die "usage: link-xy <ip_addr> <X> <Y>\n" unless
    $#ARGV == 2 &&
    $ARGV[1] =~ /^\d+$/ &&
    $ARGV[2] =~ /^\d+$/;

my $spin = SpiNN::Cmd->new (target => $ARGV[0]);
die unless $spin;

my $sv = SpiNN::Struct->new (scp => $spin);
die unless $sv;

print " X  Y   LU     0     1     2     3     4     5 \n";
print "------  --    ---   ---   ---   ---   ---   ---\n";

for (my $x = 0; $x < $ARGV[1]; $x++)
{
    for (my $y = 0; $y < $ARGV[2]; $y++)
    {
	my ($text, $lu);

	for (my $link = 0; $link < 6; $link++)
	{
	    my $d;

	    eval
	    {
		$lu = $sv->read_var ("sv.link_up", addr => [$x, $y]);

		$d = $spin->link_read ($link, 0xf5007f00, 4,
				     addr => [$x, $y],
				     unpack => "V");
	    };
	    
	    if ($@)
	    {
#		print "\n($x, $y, $link) $@";
		$text .= "   xxx";
		next;
	    }

	    my $t = sprintf "%d,%d", ($d->[0] >> 8) & 255, $d->[0] & 255;
	    $text .= sprintf " %5s", $t;
	}

	printf "%2d %2d   %02x %s\n", $x, $y, $lu, $text;
    }
}

$spin->close ();
