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

    process_args ();

    open(MYFILE, '>eth.log');
    $SIG{'INT'} = sub {close(MYFILE)};

    $i = 1;
    while (1)
    {
    	my $rc = $spin->recv_sdp (timeout => $timeout, debug => 4);

        if ($rc) {
            printf MYFILE $i.' '.$rc;
            printf $i.' '.$rc;
            $i++;
        }
        else {
            printf "# No reply\n";
#            printf "|-----------------------------------------------------------------------|\n";
#            printf "| 1| 2| 3| 4| 5| 6| 7| 8| 9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|\n";
#            printf "| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
        }
    }

    close(MYFILE);
}


main ();

#------------------------------------------------------------------------------
