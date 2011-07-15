#!/usr/bin/env ruby
# Generate jumptbl.h from enum vminst
#
in_vminst = false
instructions = Hash.new
STDIN.each do |line|
  if /^enum vminst {/ =~ line
    in_vminst = true;
  end
  next if !in_vminst
  if /^\s+(i.*)=([0-9]+)/ =~ line
    instructions[$2.to_i] = $1.clone
  elsif /^};$/ =~ line
    break
  end
end

instrlist = []
0.upto(255) do |index|
  instrlist <<
    ((instructions.has_key?(index)) ? "&&lbl_#{instructions[index]} - &&lbl_inop" :
     "&&lbl_invalid - &&lbl_inop")
end
puts instrlist.join(", ")
