#!/usr/bin/perl

##------------------------------------------------------------------------------
##
## sdp_recv	    SDP receiver program
##
## Copyright (C)    The University of Manchester - 2014
##
## Author           Steve Temple, APT Group, School of Computer Science
## Email            temples@cs.man.ac.uk
##
##------------------------------------------------------------------------------


use strict;
use warnings;

use SpiNN::SCP;


my $spin;		# SpiNNaker handle
my $timeout;	# Receive timeout
my $fh;

# Process the two arguments and open connection to SpiNNaker
# The first argument is the UDP port to listen on and the
# second is the timeout (in seconds) for the receive operation.

sub process_args
{
    die "usage: sdp_recv <port> <timeout>\n" unless
	$#ARGV == 1 &&
	$ARGV[0] =~ /^\d+$/ &&
	$ARGV[1] =~ /^\d+(\.\d+)?$/;

    $spin = SpiNN::SCP->new (port => $ARGV[0]);

    die "Failed to start receiver\n" unless $spin;

    $timeout = $ARGV[1];
}


# Main loop which waits for incoming SDP messages and prints them
# using the SpiNN::SCP debug facility

sub main
{
    my $i;
    my $str;

    process_args ();

    my $date=`date +%Y%m%d_%H%M%S`;
    chomp($date);

    open($fh, '>', "eth_".$date.".log") or die "Could not open file!";
    $SIG{'INT'} = sub {close($fh)};

    $i = 1;
    while (1)
    {
    	my $rc = $spin->recv_sdp(timeout => $timeout, debug => 0);

        if ($rc) {
            $str=$spin->{sdp_data};
            $str = substr $str, 16;
            print "$str\n";
            print $fh "$str\n";
        }
    }

    close($fh);
}


main ();

#------------------------------------------------------------------------------
