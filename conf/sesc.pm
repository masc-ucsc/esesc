# Help package to report SESC statistics
# IMPORTANT: This needs perl v5.6.1 or newer
package sesc;

#use strict;
use vars qw($VERSION $CLASS);
$VERSION = '2.0';
$CLASS = __PACKAGE__;

use warnings;
use Carp;

=head1 NAME

sesc - Package to manipulate SESC configuration files.

=head1 SYNOPSIS

 use sesc;

=head1 EXAMPLE

=head1 DESCRIPTION

=head1 AUTHORS 

Jose Renau, renau@soe.ucsc.edu

=head1 HISTORY

=cut

=head1 FUNCTION SUMMARY

=cut


#######################################
# Interface description:
#
# <number>=deMegafier(<string>)
# <string>=megafier(<number>)


=item B<new>

 my $cf = sesc->new('confFile');

Returns the sesc object corresponding to the configuration file passed
as parameter.

=cut

sub new {
  my ($proto,$file) = @_;
  my $class = ref($proto) || $proto;

  my $self = {};

  defined $file or croak "confFile must be provided";

  $self->{file} = $file;

  open(FILE,"<${file}") or croak("I could not open configuration file [${file}]");

  # Skip configuration section
  my $section = "";
  while(<FILE>) {
    last if( /^\#END/ );
    last if( /^END/ );
    next if( /^\#BEGIN/ );
    next if( /^BEGIN/ );

    my $line = $_;
    if( $line =~ /^\[([\_[:alnum:]]+)\]/ ) {
      $section = uc $1;
      next;
    }

    if( $line =~ /([[:alnum:]]+)[ \t]*=[ \t\'\"]*([^\n\'\"]+)/ ) {
      $self->{conf}{$section}[0]{uc $1} = $2;
#      printf "%s=%s [%s]\n",$1,$2,$section;
    }elsif( $line =~ /([[:alnum:]]+)\[([[:digit:]]+)\][ \t]*=[ \t\'\"]*([^\n\'\"]+)/ ) {
      $self->{conf}{$section}[$2]{uc $1} = $3;
#      printf "%s[%d]=%s [%s]\n",$1,$2,$3,$section;
    }elsif( $line =~ /([[:alnum:]]+)\[([[:digit:]]+):([[:digit:]]+)\][ \t]*=[ \t\'\"]*([^\n\'\"]+)/ ) {
      for(my $i=$2;$i<=$3;$i++) {
        $self->{conf}{$section}[$i]{uc $1} = $4;
#	printf "%s[%d]=%s [%s]\n",$1,$i,$4,$section;
      }
    }
  }
  while(<FILE>) {
    chop();

    my @fields = split(/:/);
    foreach (@fields[1..@fields-1]) {

      if( /([^:\n=]+)[ \t]*=[ \t]*([^:\n]+)/ ) {
	my $tmp = uc ($fields[0] . $1);
	if( defined $self->{res}{$tmp} ) {
#	  printf "%s:%s=%s\n",$fields[0],$1,$2;
	  #carp "The entry [" . $fields[0] . ":" . $1 ."] is redefined file[" . $self->{file} . "]. Ignoring additional entries.";
            #TODO: why is this happening in bzip? DM
	}else{
	  $self->{res}{$tmp} = $2 if( $2 ); # Only for > 0
#         printf "%s=%s\n",$tmp,$2;
	}
      }
    }
  }
  close(FILE);

  bless $self, $class;

  return $self;
}

sub DESTROY {
  my $self = shift;

  delete $self->{res};
  delete $self->{conf};
}

=item B<getResultField>

  printf "cycles=%d\n",$cf->getResultField("Proc(0)","clockTicks");

Searchs in the result file for the appropiate entry. The previous example would do
a match for the first entry with the form "Proc(0):clockTicks=10"

=cut

sub getResultField {
  my ($self,$match,$match1) = @_;

  defined $match or croak "getResultField needs at least one parameter";

  $match .= $match1 if( defined $match1 );

  return $self->{res}{uc $match};
}

sub getCkResultField {
  my ($self,$match,$match1) = @_;

  defined $match or croak "getResultField needs at least one parameter";

  $match .= $match1 if( defined $match1 );
  
  if( exists $self->{res}{uc $match} ) {
      return $self->{res}{uc $match};
  } else {
      return 0;
  }
}


=item B<getConfigEntry>

  my $cpuType = $cf->getConfigEntry(key=>"cpucore",index=>1);
  my $fetch   = $cf->getConfigEntry(key=>"fetchWidth",section=>$cpuType);

Searchs for a configuration parameter. getConfigEntry has three
parameters: key, section, and index. The "key" field (mandatory) is
the key field specified by section. If a "section" field is not
provided, it is assumed to be the main section. Additionally, an
"index" field can be provided to search for an specific dimension.

=cut

sub getConfigEntry {
  my $self  = shift;
  my %param = @_;

  defined $param{key} or croak "getConfigField requires the key field";

  my $index   = defined $param{index}  ? $param{index}   : 0;
  my $section = defined $param{section}? $param{section} : "";

#  printf "%s[%d] section[%s]\n",$param{key},$index,$section;

  return $self->{conf}{uc $section}[$index]{uc $param{key}};
}

sub configEntryExists {
  my $self  = shift;
  my %param = @_;

  defined $param{key} or croak "getConfigField requires the key field";

  my $index   = defined $param{index}  ? $param{index}   : 0;
  my $section = defined $param{section}? $param{section} : "";

#  printf "%s[%d] section[%s]\n",$param{key},$index,$section;

  return exists $self->{conf}{uc $section}[$index]{uc $param{key}};
}

#######################################
# Dissasembler section code based in dismips.c
#
# This is a copy of the copyright from dismips.c
#	Created by Giampiero Caprino
#	Backer Street Software
#
#	This software is part of REC, the reverse engineering compiler.
#	You are free to use, copy, modify and distribute this software.
#	If you fix bugs and or extend this software, I
#	will be happy to include your changes in the
#	most current version of the source.
#	just send mail to caprino@netcom.com
#


my $opp;
my $curPC;
my $itabSize;

my @itab0;
my @itab1;
my @itab2;
my @itab3;
my @itab4;
my @itab5;
my @itab6;

my $op26 = sub {
  my ($iPos, $val) = @_;

  if(($val >> 26) == $itab1[$iPos]) {
    return(1);
  }

  return(0);
};


my $op32 = sub {
  my ($iPos, $val) = @_;

  if($val == $itab1[$iPos]) {
    return(1);
  }

  return(0);
};

my $op26_16 = sub {
  my ($iPos, $val) = @_;

  if(($val >> 26) == $itab1[$iPos]
     && (($val >> 16) & 0x1F) == $itab2[$iPos]) {
    return(1);
  }

  return(0);
};

my $op26__16 = sub {
  my ($iPos, $val) = @_;

  if(($val >> 26) == $itab1[$iPos]
     && ($val & 0xFFFF) == $itab2[$iPos] ) {
    return(1);
  }

  return(0);
};

my $op21_6 = sub {
  my ($iPos, $val) = @_;

  if(($val >> 21) == $itab1[$iPos]
     && ($val & 0x3F) == $itab2[$iPos] ) {
    return(1);
  }

  return(0);
};

my $op5___11 = sub {
  my ($iPos, $val) = @_;

  if(($val >> 26) == $itab1[$iPos]
     && ($val & 0x7FF) == $itab2[$iPos] ) {
    return(1);
  }

  return(0);
};

my $op26_5 = sub {
  my ($iPos, $val) = @_;

  if(($val >> 26) == $itab1[$iPos]
     && ($val & 0x3F) == $itab2[$iPos] ) {
    return(1);
  }

  return(0);
};

my $op26_16_11 = sub {
  my ($iPos, $val) = @_;

  if(($val >> 26) == $itab1[$iPos]
     && ($val & 0x001F07FF) == $itab2[$iPos] ) {
    return(1);
  }

  return(0);
};

my $op26_21 = sub {
  my ($iPos, $val) = @_;

  if(($val >> 26) == $itab1[$iPos]
     && ($val & 0x001FFFFF) == $itab2[$iPos]) {
    return(1);
  }

  return(0);
};

my $op_r21 = sub {
  my ($iPos, $val) = @_;

  $opp = " r" . (($val >> 21) & 0x1F);

  return(1);
};

my $op_r16 = sub {
  my ($iPos, $val) = @_;

  $opp = " r" . (($val >> 16) & 0x1F);

  return(1);
};

my $op_r11 = sub {
  my ($iPos, $val) = @_;

  $opp = " r" . (($val >> 11) & 0x1F);

  return(1);
};

my $op_r6 = sub {
  my ($iPos, $val) = @_;

  $opp = " r" . (($val >> 6) & 0x1F);

  return(1);
};

my $op_i16 = sub {
  my ($iPos, $val) = @_;


  $opp = sprintf(" #0x%x", ($val & 0xFFFF));

  return(1);
};

my $op_u16 = sub {
  my ($iPos, $val) = @_;

  $opp = sprintf(" #0x%x", ($val & 0xFFFF));

  return(1);
};

my $op_i6 = sub {
  my ($iPos, $val) = @_;

  $opp = sprintf(" #0x%x", (($val >> 6) & 0x1F));

  return(1);
};

my $op_h16 = sub {
  my ($iPos, $val) = @_;

  $opp = sprintf(" #0x%x", ((($val << 16) & 0xFFFFFFFF))>>16);

  return(1);
};

my $op_b21 = sub {
  my ($iPos, $val) = @_;

  my $lv;
  if( $val >> 31 ) {
    $lv = ($val & 0xFFFF) - 0xFFFF -1;
    $lv = 0 if( $lv < -0xFFFF );
  }

  $opp = sprintf(" %d(r%d)", ($lv), (($val>>21) & 0x1F));

  return(1);
};

my $op_p16 = sub {
  my ($iPos, $val) = @_;

  $opp = sprintf(" 0x%x", ($curPC + 4 + (($val & 0xFFFF)<<2)));

  return(1);
};

my $op_p26 = sub {
  my ($iPos, $val) = @_;

  my $lv = $val & 0x03FFFFFF;
  $lv <<= 2;
  $lv |= (($curPC + 4) & 0xF0000000);

  $opp = sprintf(" 0x%x", $lv);

  return(1);
};

my $op_null = sub {
  my ($iPos, $val) = @_;

  return(0);
};

sub initITAB {
  my $pos=0;

  $itab0[$pos] ="add";
  $itab1[$pos] =0;
  $itab2[$pos] =0x20;
  $itab3[$pos] =$op5___11;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_r16;
  $pos++;

  $itab0[$pos] ="addi";
  $itab1[$pos] =8;
  $itab2[$pos] =0x00;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_i16;
  $pos++;

  $itab0[$pos] ="addiu";
  $itab1[$pos] =9;
  $itab2[$pos] =0x00;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_i16;
  $pos++;

  $itab0[$pos] ="addu";
  $itab1[$pos] =0;
  $itab2[$pos] =0x21;
  $itab3[$pos] =$op5___11;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_r16;
  $pos++;

  $itab0[$pos] ="and";
  $itab1[$pos] =0;
  $itab2[$pos] =0x24;
  $itab3[$pos] =$op5___11;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_r16;
  $pos++;

  $itab0[$pos] ="andi";
  $itab1[$pos] =0xc;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_u16;
  $pos++;

  $itab0[$pos] ="beq";
  $itab1[$pos] =0x4;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_r16;
  $itab6[$pos] =$op_p16;
  $pos++;

  $itab0[$pos] ="beql";
  $itab1[$pos] =0x14;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_r16;
  $itab6[$pos] =$op_p16;
  $pos++;

  $itab0[$pos] ="bgez";
  $itab1[$pos] =0x1;
  $itab2[$pos] =0x1;
  $itab3[$pos] =$op26_16;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_p16;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="bgezal";
  $itab1[$pos] =0x1;
  $itab2[$pos] =0x11;
  $itab3[$pos] =$op26_16;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_p16;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="bgezall";
  $itab1[$pos] =0x1;
  $itab2[$pos] =0x13;
  $itab3[$pos] =$op26_16;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_p16;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="bgezl";
  $itab1[$pos] =0x1;
  $itab2[$pos] =0x03;
  $itab3[$pos] =$op26_16;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_p16;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="bgtz";
  $itab1[$pos] =0x7;
  $itab2[$pos] =0x0;
  $itab3[$pos] =$op26_16;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_p16;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="bgtzl";
  $itab1[$pos] =0x17;
  $itab2[$pos] =0x0;
  $itab3[$pos] =$op26_16;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_p16;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="blez";
  $itab1[$pos] =0x6;
  $itab2[$pos] =0x0;
  $itab3[$pos] =$op26_16;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_p16;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="blezl";
  $itab1[$pos] =0x16;
  $itab2[$pos] =0x0;
  $itab3[$pos] =$op26_16;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] = $op_p16;
  $itab6[$pos] = $op_null;
  $pos++;

  $itab0[$pos] ="bltz";
  $itab1[$pos] =0x1;
  $itab2[$pos] =0x0;
  $itab3[$pos] =$op26_16;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_p16;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="bltzal";
  $itab1[$pos] =0x1;
  $itab2[$pos] =0x10;
  $itab3[$pos] =$op26_16;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_p16;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="bltzall";
  $itab1[$pos] =0x1;
  $itab2[$pos] =0x12;
  $itab3[$pos] =$op26_16;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_p16;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="bne";
  $itab1[$pos] =0x5;
  $itab2[$pos] =0x0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_r16;
  $itab6[$pos] =$op_p16;
  $pos++;

  $itab0[$pos] ="bnel";
  $itab1[$pos] =0x15;
  $itab2[$pos] =0x0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_r16;
  $itab6[$pos] =$op_p16;
  $pos++;

  $itab0[$pos] ="div";
  $itab1[$pos] =0;
  $itab2[$pos] =0x1a;
  $itab3[$pos] =$op26__16;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_r16;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="divu";
  $itab1[$pos] =0;
  $itab2[$pos] =0x1b;
  $itab3[$pos] =$op26__16;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_r16;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="j";
  $itab1[$pos] =2;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_p26;
  $itab5[$pos] =$op_null;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="jal";
  $itab1[$pos] =3;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_p26;
  $itab5[$pos] =$op_null;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="jalr";
  $itab1[$pos] =0;
  $itab2[$pos] =0x9;
  $itab3[$pos] =$op26_16_11;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="jr";
  $itab1[$pos] =0;
  $itab2[$pos] =0x8;
  $itab3[$pos] =$op26_21;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_null;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="lb";
  $itab1[$pos] =0x20;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="lbu";
  $itab1[$pos] =0x24;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="lh";
  $itab1[$pos] =0x21;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="lhu";
  $itab1[$pos] =0x25;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="ll";
  $itab1[$pos] =0x30;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="lui";
  $itab1[$pos] =0x0F;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_h16;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="lw";
  $itab1[$pos] =0x23;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="lwu";
  $itab1[$pos] =0x2F;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="lwl";
  $itab1[$pos] =0x22;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="lwr";
  $itab1[$pos] =0x26;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="mult";
  $itab1[$pos] =0;
  $itab2[$pos] =0x18;
  $itab3[$pos] =$op26__16;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_r16;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="multu";
  $itab1[$pos] =0;
  $itab2[$pos] =0x19;
  $itab3[$pos] =$op26__16;
  $itab4[$pos] =$op_r21;
  $itab5[$pos] =$op_r16;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="nor";
  $itab1[$pos] =0;
  $itab2[$pos] =0x27;
  $itab3[$pos] =$op5___11;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_r16;
  $pos++;

  $itab0[$pos] ="or";
  $itab1[$pos] =0;
  $itab2[$pos] =0x25;
  $itab3[$pos] =$op5___11;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_r16;
  $pos++;

  $itab0[$pos] ="ori";
  $itab1[$pos] =0xd;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_u16;
  $pos++;

  $itab0[$pos] ="sb";
  $itab1[$pos] =0x28;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="sc";
  $itab1[$pos] =0x38;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="sh";
  $itab1[$pos] =0x29;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="sll";
  $itab1[$pos] =0;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26_5;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r16;
  $itab6[$pos] =$op_i6;
  $pos++;

  $itab0[$pos] ="sllv";
  $itab1[$pos] =0;
  $itab2[$pos] =4;
  $itab3[$pos] =$op5___11;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r16;
  $itab6[$pos] =$op_r21;
  $pos++;

  $itab0[$pos] ="slt";
  $itab1[$pos] =0;
  $itab2[$pos] =0x2a;
  $itab3[$pos] =$op5___11;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_r16;
  $pos++;

  $itab0[$pos] ="slti";
  $itab1[$pos] =0xa;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_i16;
  $pos++;

  $itab0[$pos] ="sltiu";
  $itab1[$pos] =0xb;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_i16;
  $pos++;

  $itab0[$pos] ="sltu";
  $itab1[$pos] =0;
  $itab2[$pos] =0x2b;
  $itab3[$pos] =$op5___11;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_r16;
  $pos++;

  $itab0[$pos] ="sra";
  $itab1[$pos] =0;
  $itab2[$pos] =3;
  $itab3[$pos] =$op21_6;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r16;
  $itab6[$pos] =$op_i6;
  $pos++;

  $itab0[$pos] ="srav";
  $itab1[$pos] =0;
  $itab2[$pos] =7;
  $itab3[$pos] =$op5___11;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r16;
  $itab6[$pos] =$op_r21;
  $pos++;

  $itab0[$pos] ="srl";
  $itab1[$pos] =0;
  $itab2[$pos] =2;
  $itab3[$pos] =$op21_6;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r16;
  $itab6[$pos] =$op_i6;
  $pos++;

  $itab0[$pos] ="srlv";
  $itab1[$pos] =0;
  $itab2[$pos] =6;
  $itab3[$pos] =$op5___11;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r16;
  $itab6[$pos] =$op_r21;
  $pos++;

  $itab0[$pos] ="sub";
  $itab1[$pos] =0;
  $itab2[$pos] =0x22;
  $itab3[$pos] =$op5___11;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_r16;
  $pos++;

  $itab0[$pos] ="subu";
  $itab1[$pos] =0;
  $itab2[$pos] =0x23;
  $itab3[$pos] =$op5___11;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_r16;
  $pos++;

  $itab0[$pos] ="sw";
  $itab1[$pos] =0x2b;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="swl";
  $itab1[$pos] =0x2a;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="swr";
  $itab1[$pos] =0x2e;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="sync";
  $itab1[$pos] =0xf;
  $itab2[$pos] =0;
  $itab3[$pos] =$op32;
  $itab4[$pos] =$op_null;
  $itab5[$pos] =$op_null;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="syscall";
  $itab1[$pos] =0xc;
  $itab2[$pos] =0;
  $itab3[$pos] =$op32;
  $itab4[$pos] =$op_null;
  $itab5[$pos] =$op_null;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="xor";
  $itab1[$pos] =0;
  $itab2[$pos] =0x26;
  $itab3[$pos] =$op5___11;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_r16;
  $pos++;

  $itab0[$pos] ="xori";
  $itab1[$pos] =0x0e;
  $itab2[$pos] =0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_r21;
  $itab6[$pos] =$op_u16;
  $pos++;

  $itab0[$pos] ="mfhi";
  $itab1[$pos] =0x0;
  $itab2[$pos] =0x10;
  $itab3[$pos] =$op26_5;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_null;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="mflo";
  $itab1[$pos] =0x0;
  $itab2[$pos] =0x12;
  $itab3[$pos] =$op26_5;
  $itab4[$pos] =$op_r11;
  $itab5[$pos] =$op_null;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="ldc1";
  $itab1[$pos] =0x35;
  $itab2[$pos] =0x0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="ldc2";
  $itab1[$pos] =0x36;
  $itab2[$pos] =0x0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="sdc1";
  $itab1[$pos] =0x3D;
  $itab2[$pos] =0x0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="sdc2";
  $itab1[$pos] =0x3E;
  $itab2[$pos] =0x0;
  $itab3[$pos] =$op26;
  $itab4[$pos] =$op_r16;
  $itab5[$pos] =$op_b21;
  $itab6[$pos] =$op_null;
  $pos++;

  $itab0[$pos] ="mult.s";
  $itab1[$pos] =0x230;
  $itab2[$pos] =0x2;
  $itab3[$pos] =$op21_6;
  $itab4[$pos] =$op_r6;
  $itab5[$pos] =$op_r11;
  $itab6[$pos] =$op_r16;
  $pos++;

  $itab0[$pos] ="mult.d";
  $itab1[$pos] =0x231;
  $itab2[$pos] =0x2;
  $itab3[$pos] =$op21_6;
  $itab4[$pos] =$op_r6;
  $itab5[$pos] =$op_r11;
  $itab6[$pos] =$op_r16;
  $pos++;

  $itab0[$pos] ="add.s";
  $itab1[$pos] =0x230;
  $itab2[$pos] =0x0;
  $itab3[$pos] =$op21_6;
  $itab4[$pos] =$op_r6;
  $itab5[$pos] =$op_r11;
  $itab6[$pos] =$op_r16;
  $pos++;


  $itab0[$pos] ="add.d";
  $itab1[$pos] =0x231;
  $itab2[$pos] =0x0;
  $itab3[$pos] =$op21_6;
  $itab4[$pos] =$op_r6;
  $itab5[$pos] =$op_r11;
  $itab6[$pos] =$op_r16;
  $pos++;

  $itabSize = $pos;
}

sub mipsDis {
  my ($addr,$val) = @_;

  my $cad = "Unknown";

  $curPC = $addr;
  $opp = "";

  initITAB();

  for(my $i=0;$i<$itabSize;$i++) {

    if( $itab3[$i]($i,$val) ) {
      my $prev = 0;

      $cad = $itab0[$i];

      if( $itab4[$i]($i,$val)) {
	$cad .= "," if( $prev );
	$cad .= $opp;
	$prev =1;
      }
      if( $itab5[$i]($i,$val)) {
	$cad .= "," if( $prev );
	$cad .= $opp;
	$prev =1;
      }
      if( $itab6[$i]($i,$val)) {
	$cad .= "," if( $prev );
	$cad .= $opp;
	$prev =1;
      }

      last;
    }
  }

  return $cad;
}
