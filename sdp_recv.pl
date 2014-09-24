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
            #Replace any %% with %. Seems like not all % were %%
            $rc =~ s/%%/%/g;
            #Replace all % with %%
            $rc =~ s/%/%%/g;
            printf MYFILE $i.' '.$rc;
            printf $i.' '.$rc;
            $i++;
        }
#        else {
#             printf "# No reply\n";
#            printf "  |---------------------------- Board No ---------------------------------|\n";
#            printf "  |-----------------------------------------------------------------------|\n";
#            printf "  | 1| 2| 3| 4| 5| 6| 7| 8| 9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|\n";
#            printf "  |--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|\n";
#            printf " 1| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0| 0|\n";
#            printf " 2| 0| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf " 3| 0| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf " 4| 0| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf " 5| 0| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf " 6| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf " 7| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf " 8| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf " 9| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "10| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "11| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "12| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "13| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "14| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "15| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "16| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "17| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "18| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "19| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "20| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "21| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "22| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "23| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "24| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "25| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "26| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "27| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "28| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "29| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "30| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "31| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "32| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "33| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "34| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "35| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "36| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "37| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "38| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "39| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "40| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "41| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "42| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "43| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "44| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "45| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "46| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "47| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "48| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .| .|\n";
#            printf "  | -| /| -| -| -| -| -| -| -| -| -| -| -| -| -| -| -| -| -| -| -| -| -| -|\n";
#        }
    }

    close(MYFILE);
}


main ();

#------------------------------------------------------------------------------
