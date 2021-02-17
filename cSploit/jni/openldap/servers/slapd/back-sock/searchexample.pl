#!/usr/bin/perl -w -T
# $OpenLDAP$
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 2007-2014 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.
##
## ACKNOWLEDGEMENTS:
## This work was initially developed by Brian Candler for inclusion
## in OpenLDAP Software.

# See: http://search.cpan.org/dist/Net-Server/

package ExampleDB;

use strict;
use vars qw(@ISA);
use Net::Server::PreFork; # any personality will do

@ISA = qw(Net::Server::PreFork);

ExampleDB->run(
  port=>"/tmp/example.sock|unix"
  #conf_file=>"/etc/example.conf"
);
exit;

### over-ridden subs below
# The protocol is the same as back-shell

sub process_request {
  my $self = shift;

  eval {

    local $SIG{ALRM} = sub { die "Timed Out!\n" };
    my $timeout = 30; # give the user 30 seconds to type a line
    alarm($timeout);

    my $request = <STDIN>;
    
    if ($request eq "SEARCH\n") {
      my %req = ();
      while (my $line = <STDIN>) {
        chomp($line);
        last if $line eq "";
        if ($line =~ /^([^:]+):\s*(.*)$/) { # FIXME: handle base64 encoded
          $req{$1} = $2;
        }
      }
      #sleep(2);  # to test concurrency
      print "dn: cn=test, dc=example, dc=com\n";
      print "cn: test\n";
      print "objectclass: cnobject\n";
      print "\n";
      print "RESULT\n";
      print "code: 0\n";
      print "info: answered by process $$\n";      
    }
    else {
      print "RESULT\n";
      print "code: 53\n";  # unwillingToPerform
      print "info: I don't implement $request";
    }

  };

  return unless $@;
  if( $@=~/timed out/i ){
    print "RESULT\n";
    print "code: 3\n"; # timeLimitExceeded
    print "info: Timed out\n";
  }
  else {
    print "RESULT\n";
    print "code: 1\n"; # operationsError
    print "info: $@\n"; # FIXME: remove CR/LF
  }

}

1;
