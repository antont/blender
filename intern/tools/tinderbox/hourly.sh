#!/bin/sh
# $Id$
#
# ***** BEGIN GPL/BL DUAL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version. The Blender
# Foundation also sells licenses for use in proprietary software under
# the Blender License.  See http://www.blender.org/BL/ for information
# about this.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
# All rights reserved.
#
# The Original Code is: all of this file.
#
# Contributor(s): none yet.
#
# ***** END GPL/BL DUAL LICENSE BLOCK *****
#
# This script is to be executed by cron every hour
# Note: install this script from intern/tools/tinderbox
#
# This script removes obsolete executables

MAXMINUTES=120
export PATH=$PATH:/usr/local/bin
BASEDIR='/export/develop/tinderbox'

cd $BASEDIR
if [ $? -ne 0 ]; then
	echo "cd $BASEDIR failed in `pwd`"
	exit 1
fi

find 20* -type f -mmin +$MAXMINUTES -exec rm {} \;

