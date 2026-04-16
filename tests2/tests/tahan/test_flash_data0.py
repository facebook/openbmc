#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#


import unittest

from common.base_flash_data0_test import BaseFlashData0Test
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class FlashData0Test(BaseFlashData0Test, unittest.TestCase):
    def set_data0_info(self):
        self.data0_dev = "/dev/ubi0_0"
        self.data0_mountpoint = "/mnt/data"
        self.data0_fs_type = "ubifs"
        self.data0_size_mb = 64
