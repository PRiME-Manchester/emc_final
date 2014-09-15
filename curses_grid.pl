#!/usr/bin/perl

use strict;
use Curses::UI;
use Curses::UI::Grid;

my $debug = 0;

# Create the root object.
my $cui = new Curses::UI ( 
    -color_support => 1,
    -clear_on_exit => 1, 
    -debug => $debug,
);

#create_promote_deps_window();
$cui->set_binding( \&exit_dialog , "\cQ");
$cui->mainloop();

sub exit_dialog {
    my $return = $cui->dialog(
        -message   => "Do you really want to quit?",
        -title     => "Confirm",
        -buttons   => ['yes', 'no'],
    );

    exit(0) if $return;
}

my $win = $cui->add('window_id', 'Window');
my $grid =$win->add(
    'mygrid',
    'Grid',
    -rows    => 3,
    -columns => 5,
    );

# set header desc 
$grid->set_label("cell$_", "Head $_")
  for (1 .. 5);

# add some data
$grid->set_cell_value("row1", "cell$_", "value $_")
  for 1 .. 5;
my $val = $grid->get_value("row1", "cell2");