# This is a sample Perl module for the OpenLDAP server slapd.
# $OpenLDAP$
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 1998-2014 The OpenLDAP Foundation.
## Portions Copyright 1999 John C. Quillan.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.

# Usage: Add something like this to slapd.conf:
#
#	database	perl
#	suffix		"o=AnyOrg,c=US"
#	perlModulePath	/directory/containing/this/module
#	perlModule	SampleLDAP
#
# See the slapd-perl(5) manual page for details.
#
# This demo module keeps an in-memory hash {"DN" => "LDIF entry", ...}
# built in sub add{} & co.  The data is lost when slapd shuts down.

package SampleLDAP;
use strict;
use warnings;
use POSIX;

$SampleLDAP::VERSION = '1.01';

sub new {
    my $class = shift;

    my $this = {};
    bless $this, $class;
    print {*STDERR} "Here in new\n";
    print {*STDERR} 'Posix Var ' . BUFSIZ . ' and ' . FILENAME_MAX . "\n";
    return $this;
}

sub init {
    return 0;
}

sub search {
    my $this = shift;
    my ( $base, $scope, $deref, $sizeLim, $timeLim, $filterStr, $attrOnly,
        @attrs )
      = @_;
    print {*STDERR} "====$filterStr====\n";
    $filterStr =~ s/\(|\)//gm;
    $filterStr =~ s/=/: /m;

    my @match_dn = ();
    for my $dn ( keys %{$this} ) {
        if ( $this->{$dn} =~ /$filterStr/imx ) {
            push @match_dn, $dn;
            last if ( scalar @match_dn == $sizeLim );

        }
    }

    my @match_entries = ();

    for my $dn (@match_dn) {
        push @match_entries, $this->{$dn};
    }

    return ( 0, @match_entries );

}

sub compare {
    my $this = shift;
    my ( $dn, $avaStr ) = @_;
    my $rc = 5;    # LDAP_COMPARE_FALSE

    $avaStr =~ s/=/: /m;

    if ( $this->{$dn} =~ /$avaStr/im ) {
        $rc = 6;    # LDAP_COMPARE_TRUE
    }

    return $rc;
}

sub modify {
    my $this = shift;

    my ( $dn, @list ) = @_;

    while ( @list > 0 ) {
        my $action = shift @list;
        my $key    = shift @list;
        my $value  = shift @list;

        if ( $action eq 'ADD' ) {
            $this->{$dn} .= "$key: $value\n";

        }
        elsif ( $action eq 'DELETE' ) {
            $this->{$dn} =~ s/^$key:\s*$value\n//im;

        }
        elsif ( $action eq 'REPLACE' ) {
            $this->{$dn} =~ s/$key: .*$/$key: $value/im;
        }
    }

    return 0;
}

sub add {
    my $this = shift;

    my ($entryStr) = @_;

    my ($dn) = ( $entryStr =~ /dn:\s(.*)$/m );

    #
    # This needs to be here until a normalized dn is
    # passed to this routine.
    #
    $dn = uc $dn;
    $dn =~ s/\s*//gm;

    $this->{$dn} = $entryStr;

    return 0;
}

sub modrdn {
    my $this = shift;

    my ( $dn, $newdn, $delFlag ) = @_;

    $this->{$newdn} = $this->{$dn};

    if ($delFlag) {
        delete $this->{$dn};
    }
    return 0;

}

sub delete {
    my $this = shift;

    my ($dn) = @_;

    print {*STDERR} "XXXXXX $dn XXXXXXX\n";
    delete $this->{$dn};
    return 0;
}

sub config {
    my $this = shift;

    my (@args) = @_;
    local $, = ' - ';
    print {*STDERR} @args;
    print {*STDERR} "\n";
    return 0;
}

1;
