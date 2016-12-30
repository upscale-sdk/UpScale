/* -----------------------------------------------------------------------------
 * Author list (in alphabetic order):
 * 
 *        Eduardo Quiñones <eduardo.quinones@bsc.es>
 *        Sara Royuela <sara.royuela@bsc.es>
 *        Roberto Vargas <roberto.vargas@bsc.es>
 * 
 * -----------------------------------------------------------------------------
 * (C) Copyright 2016 Barcelona Supercomputing Center
 * 
 * This file is part of boxer (Compilation flow) of the P-SOCRATES project.
 *
 * Boxer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published 
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Boxer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with boxer. If not, see <http://www.gnu.org/licenses/>.
 * -----------------------------------------------------------------------------
 */

#include <assert.h>
#include <setjmp.h>
#include <stddef.h>

#include <stdio.h>
#include <stdlib.h>

#include "boxer.h"

struct tree {
	unsigned long *id;
	struct tree *left, *right;
	unsigned char height;
};

static jmp_buf recover;
static struct tree *root, *altroot;

void
reset_tree_node(struct tree *t)
{
	if(t->left)
		reset_tree_node(t->left);
	if(t->right)
		reset_tree_node(t->right);

	free(t);
}

void
reset_tree_ids()
{
	if(root)
		reset_tree_node(root);
	root = NULL;
}

static struct tree *
balance(struct tree *tp)
{
	struct tree *left = tp->left, *right = tp->right, *p;
	int h, hl, hr;

	assert(left || right);

	hl = (left) ? left->height : 0;
	hr = (right) ? right->height : 0;
	h = hr - hl;

	if (h > 2) {
		if (tp->right = right->left)
			hr = tp->right->height;
		right->left = tp;
		p = right;
	} else if (h < -2) {
		if (tp->left = left->left)
			hl = tp->left->height;
		left->left = tp;
		p = left;
	} else {
		p = tp;
	}

	tp->height = ((hl > hr) ? hl : hr) + 1;
	return p;
}

static struct tree *
tree(struct tree *treep, unsigned long *id)
{
	unsigned long n;

	if (!treep) {
		treep = xcalloc(1, sizeof(*treep));
		treep->id = id;
		return treep;
	}
	if (( n = *treep->id -  *id) == 0)
		longjmp(recover, 1);
	else if (n > 0)
		treep->left = tree(treep->left, id);
	else
		treep->right = tree(treep->right, id);

	return balance(treep);
}

void *
install(unsigned long *id)
{
	if (setjmp(recover))
		return NULL;
	root = tree(root, id);
	return id;
}

struct tree *
search(unsigned long id)
{
	unsigned long n;
	struct tree *tp;

	for (tp = root; tp != NULL; ) {
		if ((n = *tp->id - id) == 0)
			return tp;
		else if (n > 0)
			tp = tp->left;
		else
			tp = tp->right;
	}

	return NULL;
}

void *
lookup(unsigned long id)
{
	struct tree *tp = search(id);

	return (tp) ? tp->id : NULL;
}

