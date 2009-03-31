/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of CArc

  CArc is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
*/

/*
  The CArc memory allocator uses a fast-fits memory allocator to
  perform its allocation.  Later on, we'll modify this so that it can
  do locking and we can safely allocate and deallocate memory this way
  in a concurrent fashion.
 */

static void demote(Bhdr *parent, Bhdr *child)
{
  Bhdr *left, *right;
  int r1;

  r1 = (parent->s.left == child);
  left = child->s.left;
  right = child->s.right;
  while (left->size > child->size || right->size > child->size) {
    if (right->size > child->size) {
    } else {
    }
  }
}
