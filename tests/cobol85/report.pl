#!/usr/bin/perl -s
#
# Copyright (C) 2001-2002 Keisuke Nishida
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
# Boston, MA 02111-1307 USA

my $compile = 'cobc';
my $compile_module = 'cobc -m';

my $num_progs = 0;
my $test_skipped = 0;
my $compile_error = 0;
my $execute_error = 0;

my $total_all = 0;
my $total_pass = 0;
my $total_fail = 0;
my $total_deleted = 0;
my $total_inspect = 0;
my $total_ok = 0;

open (LOG, "> report.txt") or die;
print LOG "Filename    total pass fail deleted inspect\n";
print LOG "--------    ----- ---- ---- ------- -------\n";

foreach $in (glob("lib/*.CBL")) {
  print "$compile_module $in\n";
  system ("$compile_module $in");
}

foreach $in (sort (glob("*.{CBL,SUB}"))) {
  my $exe = $in;
  my $cmd;
  $exe =~ s/\.CBL//;
  $exe =~ s/\.SUB//;
  $cmd = "./$exe";
  if (-e "./$exe.DAT") {
    $cmd = "./$exe < $exe.DAT";
  }
  $num_progs++;
  printf LOG "%-12s", $in;
  if ($exe =~ /^..[34]0/ || $skip{$exe}) {
    $test_skipped++;
    print LOG "  ----- test skipped -----\n";
  } else {
    $copy = ($exe =~ /^SM/) ? "-I ../copy " : "";
    print "$compile $copy$in && $cmd\n";
    if (system ("$compile $copy$in") != 0) {
      $compile_error++;
      print LOG "  ===== compile error =====\n";
    } else {
      if ($in =~ /\.CBL/) {
	system ("rm -f XXXXX*");
      }
      if (system ("$cmd > $exe.out") != 0) {
	$execute_error++;
	print LOG "  ***** execute error *****\n";
      } else {
	my $total   = 0;
	my $pass    = 0;
	my $fail    = 0;
	my $deleted = 0;
	my $inspect = 0;
	if (open (PRT, "report.log")) {
	  while (<PRT>) {
	    if (/^ *([0-9]+) *OF *([0-9]+) *TESTS WERE/) {
	      $total += $2;
	      $pass += $1;
	    } elsif (/^ *([0-9NO]+) *TEST\(S\) ([A-Z]+)/) {
	      my $num = $1 eq "NO" ? 0 : $1;
	      if ($2 eq "FAILED") {
		$fail += $num;
	      } elsif ($2 eq "DELETED") {
		$deleted += $num;
	      } elsif ($2 eq "REQUIRE") {
		$inspect += $num;
	      }
	    }
	  }
	}
	printf LOG ("%5s %4s %4s %7s %7s %s\n",
		    $total, $pass, $fail, $deleted, $inspect,
		    $fail == 0 ? "OK" : "");
	$total_all += $total;
	$total_pass += $pass;
	$total_fail += $fail;
	$total_deleted += $deleted;
	$total_inspect += $inspect;
	$total_ok++ if $fail == 0;
	rename ("report.log", "$exe.log");
	unlink "$exe.out" if (-s "$exe.out" == 0);
      }
    }
  }
}

print LOG "--------    ----- ---- ---- ------- -------\n";
printf LOG ("Total       %5s %4s %4s %7s %7s\n",
	    $total_all, $total_pass, $total_fail, $total_deleted,
	    $total_inspect);
printf LOG ("%%           100.0 %4.1f %4.1f    %4.1f    %4.1f\n\n",
	    100 * $total_pass / $total_all,
	    100 * $total_fail / $total_all,
	    100 * $total_deleted / $total_all,
	    100 * $total_inspect / $total_all) if $total_all;

$num_tested = $num_progs - $test_skipped - $compile_error - $execute_error;
printf LOG ("Number of programs:    %2s\n", $num_progs);
printf LOG ("Successfully tested:   %2s (%5.2f%%)\n",
	    $num_tested, 100 * $num_tested / $num_progs);
printf LOG ("Successfully executed: %2s (%5.2f%%)\n",
	    $total_ok, 100 * $total_ok / $num_progs);
printf LOG ("Test skipped:          %2s (%5.2f%%)\n",
	    $test_skipped, 100 * $test_skipped / $num_progs);
printf LOG ("Compile error:         %2s (%5.2f%%)\n",
	    $compile_error, 100 * $compile_error / $num_progs);
printf LOG ("Execute error:         %2s (%5.2f%%)\n",
	    $execute_error, 100 * $execute_error / $num_progs);
