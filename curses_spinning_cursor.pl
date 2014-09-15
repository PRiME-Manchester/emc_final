#!/usr/bin/perl

## o) In Perl, write a program that will make the cursor appear to spin

# Using Time::HiRes to allow < 1 second delay between
# "spin" updates.
use Time::HiRes;

# Using the Curses library, so the UI is more manageable
use Curses;

$window = initscr(); # initialize the curses window

if ($window == NULL ) {
    print ("Failed to initialize Curses window\n" );
    exit(-1);
}

nodelay(1); # make the getch() non-blocking
curs_set(0); # vanish the cursor

@spin_array = qw(| / - \\);
$message = "Press 'q' to stop spinning... ";
$row = 2;
$col = length($message); # Column to draw spinner at end of message

addstr($row, 0, $message); # Show the message

# spin this thing until someone stops it
$counter = 0;
while ( 1 ) {
    $key = getch(); # get keyboard input

    # Look for user to tell it to stop
    if ( $key eq "q") {
        clear();

        # Replace spinner with our gratitude
        addstr($row, $col, "Thank you!");
        refresh();
        sleep(2);

        # clean up and exit
        delwin($window);
        endwin();
        refresh();
        exit(0);
    }

    # check on progress of background thing could be here....

    # use a reasonable wait time between spinner updates
    Time::HiRes::sleep(0.2);

    # draw the spinner
    addstr ($row, $col,"@spin_array[ $counter ]");
    addstr ($row, $col+2,"@spin_array[ $counter ]");
    addstr ($row, $col+4,"@spin_array[ $counter ]");

    # count
    if ($counter == 3) {
        $counter = 0;
    } else {
        $counter++;
    }
}