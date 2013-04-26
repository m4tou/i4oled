# Copyright 1999-2013 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=5
inherit autotools git

DESCRIPTION="A simple helper for setting and rendering OLED icons on Wacom Intuos4 tablets"
HOMEPAGE="https://github.com/PrzemoF/i4oled"

EGIT_REPO_URI="git://github.com/PrzemoF/i4oled.git"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~amd64 ~x86"

DEPEND="
	x11-libs/cairo
	x11-libs/pango
"

src_prepare() {
	eautoreconf
}

