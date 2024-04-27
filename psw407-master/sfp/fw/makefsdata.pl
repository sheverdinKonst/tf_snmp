#!/usr/bin/perl

use strict;


open(OUTPUT, "> sfp_fw.h");

print(OUTPUT "#ifndef __SFP_FW_H__\n");
print(OUTPUT "#define __SFP_FW_H__\n");
print(OUTPUT "#include \"../net/webserver/httpd-fs.h\"\n");



chdir("bin");

opendir(DIR, ".");

my @files =  grep { !/^\./ && !/(CVS|~)/ } readdir(DIR);

closedir(DIR);


my $file;



foreach $file (@files) {
  
   
    if(-d $file && $file !~ /^\./) {

      print "Processing directory $file\n";

      opendir(DIR, $file);

      my @newfiles =  grep { !/^\./ && !/(CVS|~)/ } readdir(DIR);

      closedir(DIR);

      printf "Adding files @newfiles\n";

      @files = (@files, map { $_ = "$file/$_" } @newfiles);

      next;

    }
}

my @fvars;

my @pfiles;



foreach $file (@files) {

  if(-f $file) {

    print "Adding file $file\n";

    
    open(FILE, $file) || die "Could not open file $file\n";


    #$file =~ s-^--;
	#$file =~ s-^--;

    my $fvar = $file;

    $fvar =~ s-/-_-g;

    $fvar =~ s-\.-_-g;
	
	$fvar =~ s-\--_-g;


	print(OUTPUT "static const char data_".$fvar."[] = {\n");
    print(OUTPUT "\t/* $file */\n\t");

    my $j;

    for($j = 0;	$j < length($file);	$j++) {
        printf(OUTPUT "%#.2X, ", unpack("C", substr($file, $j, 1)));
    }
    printf(OUTPUT "0,\n");
  
    
    my $i = 0;

    my $data;

    while(read(FILE, $data, 1)) {
 
       if($i == 0) {
          print(OUTPUT "\t");
        }
 
	   if(unpack("C", $data) == 0){
		  print(OUTPUT "0x00, ");
	   }
	   else{
		  printf(OUTPUT "%#.2x, ", unpack("C", $data));
	   }
       $i++;

        if($i == 16) {
			print(OUTPUT "\n");
			$i = 0;
        }
    }
    print(OUTPUT "0};\n\n");

    close(FILE);

    push(@fvars, $fvar);

    push(@pfiles, $file);
  }
}

my $i;

my $prevfile;

for($i = 0; $i < @fvars; $i++) {
    my $file = $pfiles[$i];
    my $fvar = $fvars[$i];
	
    if($i == 0) {
        $prevfile = "NULL";
    } else {
        $prevfile = "file_" . $fvars[$i - 1];
    }
   
 print(OUTPUT "const struct httpd_fsdata_file file_".$fvar."[] = {{$prevfile, data_$fvar, ");

 print(OUTPUT "data_$fvar + ". (length($file) + 1) .", ");
 print(OUTPUT "sizeof(data_$fvar) - ". (length($file) + 1) ."}};\n\n");

}

print(OUTPUT "#define SFP_FW_FS_ROOT file_$fvars[$i - 1]\n\n");
print(OUTPUT "#define SFP_FW_FS_NUMFILES $i\n");
print(OUTPUT "#endif /* __SFP_FW_H__ */\n");
