#!/usr/bin/perl

use strict;
use warnings;
use Curses::UI;
use Curses::UI::Grid;

my $debug = 0;

# Create the root object.
my $cui = new Curses::UI ( 
    -color_support => 1,
    -clear_on_exit => 1, 
    -debug => $debug,
);

create_window();
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

sub create_window {
    my ($name) = @_;

    my $win = $cui->add(
        'window',
        'Window', 
        -border       => 1, 
        -titlereverse => 0, 
        -padtop       => 0, 
        -padbottom    => 0, 
        -ipad         => 0,
        -title        => 'Spinnaker EMC Test',
    );


    my $grid = $win->add(
        'grid',
        'Grid',
        -height       => 26,
        -width        => -1,
        -editable     => 0,
        -border       => 0,
        -process_bindings => {
            CUI_TAB => undef,
        },
        # -bg       => "blue",
        # -fg       => "white",
    );

    $grid->add_cell(
        "chip",
        -width => 4,
        -label => "Chip"
    );

    $grid->add_cell(
        "1",
        -width => 2,
        -label => "1"
    );

    $grid->add_cell(
        "2",
        -width => 2,
        -label => "2"
    );

    $grid->add_cell(
        "3",
        -width => 2,
        -label => "3"
    );

    $grid->add_cell(
        "4",
        -width => 2,
        -label => "4"
    );

    $grid->add_cell(
        "5",
        -width => 2,
        -label => "5"
    );

    $grid->add_cell(
        "6",
        -width => 2,
        -label => "6"
    );

    $grid->add_cell(
        "7",
        -width => 2,
        -label => "7"
    );

    $grid->add_cell(
        "8",
        -width => 2,
        -label => "8"
    );

    $grid->add_cell(
        "9",
        -width => 2,
        -label => "9"
    );

    $grid->add_cell(
        "10",
        -width => 2,
        -label => "10"
    );

    $grid->add_cell(
        "11",
        -width => 2,
        -label => "11"
    );

    $grid->add_cell(
        "12",
        -width => 2,
        -label => "12"
    );

    $grid->add_cell(
        "13",
        -width => 2,
        -label => "13"
    );

    $grid->add_cell(
        "14",
        -width => 2,
        -label => "14"
    );

    $grid->add_cell(
        "15",
        -width => 2,
        -label => "15"
    );

    $grid->add_cell(
        "16",
        -width => 2,
        -label => "16"
    );

    $grid->add_cell(
        "17",
        -width => 2,
        -label => "17"
    );

    $grid->add_cell(
        "18",
        -width => 2,
        -label => "18"
    );

    $grid->add_cell(
        "19",
        -width => 2,
        -label => "19"
    );

    $grid->add_cell(
        "20",
        -width => 2,
        -label => "20"
    );

    $grid->add_cell(
        "21",
        -width => 2,
        -label => "21"
    );

    $grid->add_cell(
        "22",
        -width => 2,
        -label => "22"
    );

    $grid->add_cell(
        "23",
        -width => 2,
        -label => "23"
    );

    $grid->add_cell(
        "24",
        -width => 2,
        -label => "24"
    );

    my @data = (
        ['1',  'HDT-10', 'e3042b0', 'HDT-7', '6741e47', 'src/tc/b.p'],
        ['2',  'HDT-10', 'e3042b0', 'HDT-7', '6741e47', 'src/tc/a.p'],
        ['3',  'HDT-10', 'e3042b0', 'HDT-7', '6741e47', 'src/tc/c.p'],
        ['4',  'HDT-10', 'e3042b0', 'HDT-7', '66a3254', 'src/tc/c.p'],
        ['5',  'HDT-10', 'e3042b0', 'HDT-7', '66a3254', 'src/tc/b.p'],
        ['6',  'HDT-10', 'e3042b0', 'HDT-7', '66a3254', 'src/tc/a.p'],
        ['7',  'HDT-10', 'e3042b0', 'HDT-8', '8b65677', 'src/tc/e.p'],
        ['8',  'HDT-10', 'e3042b0', 'HDT-8', '8b65677', 'src/tc/d.p'],
        ['9',  'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['10', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['11', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['12', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['13', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['14', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['15', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['16', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['17', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['18', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['19', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['20', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['21', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['22', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['23', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['24', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['25', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['26', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['27', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['28', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['29', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['30', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['31', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['32', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['33', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['34', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['35', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['36', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['37', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['38', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['39', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['40', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['41', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['42', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['43', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['44', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['45', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['46', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['47', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
        ['48', 'HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
    );

    foreach my $f (@data) {
      $grid->add_row(
        undef,
        # -fg    => 'black',
        # -bg    => 'yellow',
        -cells => {
          chip => $f->[0],
          1 => $f->[1],
          2 => $f->[2],
          3 => $f->[3],
          4 => $f->[4],
        }
      );
    }

    $grid->layout();
    return $win;
}
