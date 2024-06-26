#!/usr/bin/perl

use strict;


open(OUTPUT, "> httpd-fsdata.h");

print(OUTPUT "#ifndef __HTTPD_FSDATA_H__\n");
print(OUTPUT "#define __HTTPD_FSDATA_H__\n");
print(OUTPUT "#include \"httpd-fs.h\"\n");
print(OUTPUT "#include \"eeprom.h\"\n");


chdir("httpd-fs");

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


    $file =~ s-^-/-;

    my $fvar = $file;

    $fvar =~ s-/-_-g;

    $fvar =~ s-\.-_-g;

	#exclude PSW2G_settings_backup.img
	 # for AVR, add PROGMEM here
	#if($fvar eq "_PSW2G_settings_backup_bak"){
	if($fvar eq "_PSW2G_settings_backup_bak"){
		print(OUTPUT "char data".$fvar."[] = {\n");
	}
	else{	
		print(OUTPUT "static const char data".$fvar."[] = {\n");
	}
    print(OUTPUT "\t/* $file */\n\t");

    my $j;

    for($j = 0;
 $j < length($file);
 $j++) {

        printf(OUTPUT "%#02x, ", unpack("C", substr($file, $j, 1)));

    }
    printf(OUTPUT "0,\n");

    
    
    my $i = 0;

    my $data;

    while(read(FILE, $data, 1)) {
 
       if($i == 0) {

          print(OUTPUT "\t");

        }
 
       printf(OUTPUT "%#02x, ", unpack("C", $data));

        $i++;

        if($i == 10) {

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

        $prevfile = "file" . $fvars[$i - 1];

    }
   
 print(OUTPUT "const struct httpd_fsdata_file file".$fvar."[] = {{$prevfile, data$fvar, ");

 print(OUTPUT "data$fvar + ". (length($file) + 1) .", ");
 print(OUTPUT "sizeof(data$fvar) - ". (length($file) + 1) ."}};\n\n");

}

print(OUTPUT "#define HTTPD_FS_ROOT file$fvars[$i - 1]\n\n");

print(OUTPUT "#define HTTPD_FS_NUMFILES $i\n");


print(OUTPUT "#endif /* __HTTPD_FSDATA_H__ */\n");
