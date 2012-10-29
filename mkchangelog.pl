@aLines         = split /\n/, `git log`;
$stop           = `cat VERSION`;
$is_message     = 0;
$message        = '';

print "Version :\n\n";

foreach my $line (@aLines)
{
  last if $line =~ $stop;

  if( $line =~ /^Date:.+/ )
  {
    $is_message = 1;
  }
  elsif( $line =~ /^commit .+/ )
  {
    $message = trim( $message );
    
    print " $message\n" if $message;
     
    $is_message = 0;
    $message    = '';
  }  
  elsif( $is_message && $line =~ /^\s+(.*)/ )
  {
    $message .= " ".$1    
  }        
}

sub trim($)
{
  my $string = shift;
  $string =~ s/^\s+//;
  $string =~ s/\s+$//;
  return $string;
}

sub ltrim($)
{
  my $string = shift;
  $string =~ s/^\s+//;
  return $string;
}

sub rtrim($)
{
  my $string = shift;
  $string =~ s/\s+$//;
  return $string;
}
