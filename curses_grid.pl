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

create_promote_deps_window();
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

sub create_base_window {
    my ($name) = @_;

    $cui->add(
        $name,
        'Window', 
        -border       => 1, 
        -titlereverse => 0, 
        -padtop       => 2, 
        -padbottom    => 3, 
        -ipad         => 1,
        -title        => 'CTRL-Q to quit',
    );
}

sub create_promote_deps_window {
    my ($name) = @_;

    my $win = create_base_window($name);

    my $grid = $win->add(
        'grid',
        'Grid',
        -height       => 14,
        -width        => -1,
        -editable     => 0,
        -border       => 1,
        -process_bindings => {
            CUI_TAB => undef,
        },
        # -bg       => "blue",
        # -fg       => "white",
    );

    $grid->add_cell(
        "otp",
        -width => 10,
        -label => "OTP"
    );

    $grid->add_cell(
        "commit1",
        -width => 10,
        -label => "Commit#"
    );

    $grid->add_cell(
        "otnp",
        -width => 10,
        -label => "OTNP"
    );

    $grid->add_cell(
        "commit2",
        -width => 10,
        -label => "Commit#"
    );

    $grid->add_cell(
        "overlap",
        -width => 32,
        -label => "Overlap"
    );

    my $button_callback = sub {
        my $this = shift;

        my $btn_name = $this->get();
        if ($btn_name eq "Back") {
            # give up promotion and return to promote window
            $win->focus();
        }
        elsif ($btn_name eq "PromoteWithDeps") {
        }
    };
    $win->add(
        undef,
        'Buttonbox',
        -y        => -1,
        -buttons  => [
            {
                -label   => "< Back >",
                -value   => "Back",
                -onpress => $button_callback,
            },
            {
                -label   => "< Promote w/ all deps >",
                -value   => "PromoteWithDeps",
                -onpress => $button_callback,
            },
        ],
    );

    my @data = (
        ['HDT-10', 'e3042b0', 'HDT-7', '6741e47', 'src/tc/b.p'],
        ['HDT-10', 'e3042b0', 'HDT-7', '6741e47', 'src/tc/a.p'],
        ['HDT-10', 'e3042b0', 'HDT-7', '6741e47', 'src/tc/c.p'],
        ['HDT-10', 'e3042b0', 'HDT-7', '66a3254', 'src/tc/c.p'],
        ['HDT-10', 'e3042b0', 'HDT-7', '66a3254', 'src/tc/b.p'],
        ['HDT-10', 'e3042b0', 'HDT-7', '66a3254', 'src/tc/a.p'],
        ['HDT-10', 'e3042b0', 'HDT-8', '8b65677', 'src/tc/e.p'],
        ['HDT-10', 'e3042b0', 'HDT-8', '8b65677', 'src/tc/d.p'],
        ['HDT-10', 'e3042b0', 'HDT-9', '3eefa90', 'src/tc/f.p'],
    );
    foreach my $f (@data) {

        $grid->add_row(
            undef,
            # -fg    => 'black',
            # -bg    => 'yellow',
            -cells => {
                otp     => $f->[0],
                commit1 => $f->[1],
                otnp    => $f->[2],
                commit2 => $f->[3],
                overlap => $f->[4],
            }
        );
    }

    $grid->layout();
    return $win;
}
