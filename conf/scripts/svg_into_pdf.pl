#!/usr/bin/perl -w
#####
#
#   svg_into_svg.pl
#   copyright 2003, Kevin Lindsey <kevin@kevlindev.com>
#   licensing info: http://www.kevlindev.com/license.txt
#
#####

#####
#
#   globals
#
#####
my $version = "0.3";
my $objects = [];


#####
#
#   main
#
#####
if ( @ARGV > 0 ) {
    # load PDF objects
    makeObjects();

    while ( defined(my $svgFile = shift) ) {
        my $pdfFile = $svgFile;
        
        $pdfFile =~ s/\.svgz?$//i;
        $pdfFile .= ".pdf";

        insertSVG($svgFile);
        writePDF($pdfFile);

        # remove trailing svg object for next iteration
        pop @$objects;
    }
} else {
    print <<EOF;

svg_into_pdf, v$version
Copyright 2003, Kevin Lindsey <kevin\@kevlindev.com>
Licensing info: http://www.kevlindev.com/license.txt

usage: svg_into_pdf.pl <svg-filename>+
EOF
}


#####
#
#   makeObjects
#
#####
sub makeObjects {
    while ( defined(my $ids = <DATA>) ) {
        chomp $ids;
        my ($id, $genId) = split(/\s+/, $ids);
        my $text = "";
        while ( 1 ) {
            my $line = <DATA>;
            last if $line =~ /^\s*$/;
            $text .= $line;
        }
        chomp $text;
        push @$objects, new PDFObject($id, $genId, $text);
    }
}


#####
#
#   insertSVG
#
#####
sub insertSVG {
    my $filename = shift;
    my $svg;

    {
        local $/ = undef;
        open INPUT, $filename || die "Could not open SVG file $filename: $!";
        $svg = <INPUT>;
        close INPUT;
    }

    chomp $svg;
    my $size = length($svg);
    my $obj = new PDFObject(scalar(@$objects)+1, 0, "");
    my $content = <<EOF;
<<
    /Length $size
    /Type /EmbeddedFile
    /Subtype /image#2Fsvg+xml
>>
stream
$svg
endstream
EOF
    chomp $content;
    $obj->setContent($content);
    push @$objects, $obj;
}

#####
#
#   writePDF
#
#####
sub writePDF {
    my $filename = shift;

    open OUTPUT, ">$filename" || die "Unable to create $filename: $!";

    my $oldFH = select(OUTPUT);

    writeHeader();
    my $xrefStart = writeObjects();
    writeXref();
    writeTrailer($xrefStart);

    select($oldFH);
}
    


#####
#
#   writeHeader
#
#####
sub writeHeader {
    # write PDF header
    print "\%PDF-1.4\n";
}


#####
#
#   writeObjects
#
#####
sub writeObjects {
    # output all indirect objects
    foreach my $obj ( @$objects ) {
        # set object position to current position in output file
        $obj->setOffset(tell OUTPUT);

        # output contents of object
        print $obj->toString();
    }

    return tell OUTPUT;
}


#####
#
#   writeXref
#
#####
sub writeXref {
    # create xref table
    print "xref\n";

    # output starting object id and number of objects
    print "0 ", scalar(@$objects), "\n";

    # output one dummy free object
    print "0000000000 65535 f \n";

    # output subsection entry for each object
    foreach my $obj ( @$objects ) {
        print $obj->toEntry();
    }
}


#####
#
#   writeTrailer
#
#####
sub writeTrailer {
    my $xrefStart = shift;

    # output trailer
    print "trailer\n";

    # output # of objects and root object to trailer dictionary
    print "<< /Size ", scalar(@$objects) + 1, "\n";
    print "   /Root 1 0 R\n";
    print ">>\n";


    # output start of xref table
    print "startxref\n";

    # output xref offset
    print $xrefStart, "\n";

    # output PDF footer
    print "\%\%EOF\n";
}


#####
#
#   PDFObject
#
#####
package PDFObject;
use strict;

#####
#
#   constants
#
#####
use constant ID      => 0;
use constant GEN     => 1;
use constant CONTENT => 2;
use constant OFFSET  => 3;


#####
#
#   new
#
#####
sub new {
    my $self  = shift;
    my $class = ref($self) || $self;
    my $this  = [];

    $this->[ID]      = shift;
    $this->[GEN]     = shift;
    $this->[CONTENT] = shift;
    $this->[OFFSET]  = 0;

    return bless $this, $class;
}


#####
#
#   toString
#
#####
sub toString {
    my $this = shift;

    return
        $this->[ID] . " " .
        $this->[GEN] . " obj\n" .
        $this->[CONTENT] . "\n" .
        "endobj\n";
}


#####
#
#   toEntry
#
#####
sub toEntry {
    my $this = shift;

    return
        sprintf("%010d", $this->[OFFSET]) . " " .
        sprintf("%05d", $this->[GEN]) . " " .
        "n \n";
}


#####
#
#   get/set methods
#
#####
sub getOffset  { $_[0]->[OFFSET]; }
sub setOffset  { $_[0]->[OFFSET]  = $_[1]; }
sub setContent { $_[0]->[CONTENT] = $_[1]; }


package main;

__DATA__
1 0
<< 
   /Type /Catalog 
   /Outlines 2 0 R
   /Pages 3 0 R 
   /Names 8 0 R 
>> 

2 0
<< /Type /Outlines
   /Count 0
>>

3 0
<< /Type /Pages
   /Kids [4 0 R]
   /Count 1
>>

4 0
<< /Type /Page
   /Parent 3 0 R
   /MediaBox [0 0 612 792]
   /Contents 5 0 R
   /Resources << /ProcSet 6 0 R
                 /Font << /F1 7 0 R >>
              >>
>>

5 0
<< /Length 76 >>
stream
    BT
        /F1 48 Tf
        36 720 Td
        (SVG in a PDF) Tj
    ET
endstream

6 0
    [/PDF /Text]

7 0
<<  /Type /Font
    /Subtype /Type1
    /Name /F1
    /BaseFont /Helvetica
    /Encoding /MacRomanEncoding
>>

8 0
<< 
    /AlternatePresentations 11 0 R 
    /JavaScript 9 0 R
>> 

9 0
<< 
    /Names [ (OnLoadJavaScript)<< /S /JavaScript /JS 10 0 R >> ] 
>>

10 0
<<  /Length 148 >>
stream
if (alternatePresentations['MySVGFile.svg'] != null ) {
    app.fs.isFullScreen = false;
    alternatePresentations['MySVGFile.svg'].start(null);
}
endstream

11 0
<< 
    /Names [ (MySVGFile.svg)12 0 R ] 
>> 

12 0
<< 
    /Type /SlideShow 
    /Subtype /Embedded 
    /Resources 13 0 R 
    /StartResource (MySVGFile.svg)
>> 

13 0
<< 
    /Names [ (MySVGFile.svg)14 0 R ] 
>> 

14 0
<< 
    /Type /FileSpec 
    /F (MySVGFile.svg)
    /EF 15 0 R 
>> 

15 0
<< 
    /F 16 0 R 
>> 

