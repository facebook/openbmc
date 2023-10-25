/*
 * libusb example program to list devices on the bus
 * Copyright © 2007 Daniel Drake <dsd@gentoo.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "bios-usb-update.hpp"

#include <ctype.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iomanip>

constexpr size_t USB_PKT_SIZE = 0x200;
constexpr size_t USB_DAT_SIZE = (USB_PKT_SIZE - USB_PKT_HDR_SIZE);
constexpr size_t BIOS_UPDATE_BLK_SIZE = (64 * 1024);
constexpr size_t BIOS_VERIFY_PKT_SIZE = (32 * 1024);

constexpr size_t UPDATE_BIOS = 0;
constexpr int MAX_CHECK_DEVICE_TIME = 8;
constexpr int NUM_ATTEMPTS = 5;
constexpr uint16_t SB_USB_VENDOR_ID = 0x1D6B;
constexpr uint16_t SB_USB_PRODUCT_ID = 0x0104;

enum
{
    // BIC to BMC
    USB_INPUT_PORT = 0x3,
    USB_OUTPUT_PORT = 0x82,
};



int send_bic_usb_packet(usb_dev *udev, bic_usb_packet *pkt)
{
    const int transferlen = pkt->length + USB_PKT_HDR_SIZE;
    int transferred = 0;

    for (size_t i = 0; i < MAX_RETRY_TIME; i++)
    {
        int ret =
            libusb_bulk_transfer(udev->handle, udev->epaddr, (uint8_t *)pkt,
                                 transferlen, &transferred, 3000);
        if (((ret != 0) || (transferlen != transferred)))
        {
            std::cerr << "Error in transferring data! err =" << ret
                      << "and transferred = " << transferred
                      << ", expected data length" << transferlen << "\n";
            std::cerr << "Retry since " << libusb_error_name(ret) << "\n";
            sleep(0.1);
        }
        else
        {
            return 0;
        }
    }

    return -1;
}

int receive_bic_usb_packet(usb_dev *udev, bic_usb_packet *pkt)
{
    const int receivelen = USB_PKT_RES_HDR_SIZE + IANA_ID_SIZE;
    int received = 0;
    int total_received = 0;
    bic_usb_res_packet *res_hdr = (bic_usb_res_packet *)pkt;

    for (size_t i = 0; i < MAX_RETRY_TIME; i++)
    {
        int ret =
            libusb_bulk_transfer(udev->handle, udev->epaddr, (uint8_t *)pkt,
                                 receivelen, &received, 0);
        if (ret != 0)
        {
            std::cerr << "Error in receiving data! err = " << ret << "("
                      << libusb_error_name(ret) << ")\n";
            sleep(0.1);
        }
        else
        {
            total_received += received;
            if (total_received >= receivelen)
            {
                // Return CC code
                return res_hdr->cc;
            }

            // Expected data may not received completely in one bulk transfer
            // continue to get remaining data
            pkt += received;
        }
    }

    return -1;
}

int bic_init_usb_dev([[maybe_unused]] uint8_t slot_id, usb_dev *udev,
                     const uint16_t product_id, const uint16_t vendor_id)
{
    int ret = 0;
    int index = 0;
    ssize_t cnt;
    char found = 0;

    ret = libusb_init(nullptr);
    if (ret < 0)
    {
        std::cerr << "BIOS update : Failed to initialise libusb\n";
        goto error_exit;
    }

    for (size_t recheck = 0; recheck < MAX_CHECK_DEVICE_TIME; recheck++)
    {
        cnt = libusb_get_device_list(nullptr, &udev->devs);
        if (cnt < 0)
        {
            std::cerr << "BIOS update : There are no USB devices on bus\n";
            goto error_exit;
        }
        index = 0;
        while ((udev->dev = udev->devs[index++]) != nullptr)
        {
            ret = libusb_get_device_descriptor(udev->dev, &udev->desc);
            if (ret < 0)
            {
                std::cerr << "BIOS update : Failed to get device descriptor -- "
                             "exit\n";
                libusb_free_device_list(udev->devs, 1);
                goto error_exit;
            }

            ret = libusb_open(udev->dev, &udev->handle);
            if (ret < 0)
            {
                std::cerr << "BIOS update : Error opening device -- exit\n";
                libusb_free_device_list(udev->devs, 1);
                goto error_exit;
            }

            if ((vendor_id == udev->desc.idVendor) &&
                (product_id == udev->desc.idProduct))
            {
                ret = libusb_get_string_descriptor_ascii(
                    udev->handle, udev->desc.iManufacturer,
                    (unsigned char *)udev->manufacturer,
                    sizeof(udev->manufacturer));
                if (ret < 0)
                {
                    std::cerr << "BIOS update : Error get Manufacturer string "
                                 "descriptor -- exit\n";
                    libusb_free_device_list(udev->devs, 1);
                    goto error_exit;
                }

                ret = libusb_get_port_numbers(udev->dev, udev->path,
                                              sizeof(udev->path));

                if (ret < 0)
                {
                    std::cerr << "BIOS update : Error get port number\n";
                    libusb_free_device_list(udev->devs, 1);
                    goto error_exit;
                }

                /*
                 slot 1,3,5,7 pass one hub
                 slot 2,4,6,8 pass two hubs
                */
                if (ret == 3 && (udev->path[1] * 2 - 1) == slot_id) {
                  std::cout << "slot" << (unsigned) slot_id << " found!\n";
                } else if (ret == 4 && udev->path[1] == 1 /*is HUB2*/ && (udev->path[2] * 2) == slot_id) {
                  std::cout << "slot" << (unsigned) slot_id << " pass two hubs found!\n";
                } else {
                  continue;
                }

                ret = libusb_get_string_descriptor_ascii(
                    udev->handle, udev->desc.iProduct,
                    (unsigned char *)udev->product, sizeof(udev->product));
                if (ret < 0)
                {
                    std::cerr << "BIOS update : Error get Product string "
                                 "descriptor -- exit\n";
                    libusb_free_device_list(udev->devs, 1);
                    goto error_exit;
                }
                found = 1;
                break;
            } // is server board
        }

        if (found != 1)
        {
            sleep(3);
        }
        else
        {
            break;
        }
    }

    if (found == 0)
    {
        std::cerr << "BIOS update : Device NOT found -- exit\n";
        libusb_free_device_list(udev->devs, 1);
        goto error_exit;
    }

    ret = libusb_get_configuration(udev->handle, &udev->config);
    if (ret != 0)
    {
        std::cerr
            << "BIOS update : Error in libusb_get_configuration -- exit\n";
        libusb_free_device_list(udev->devs, 1);
        goto error_exit;
    }

    if (udev->config != 1)
    {
        ret = libusb_set_configuration(udev->handle, 1);
        if (ret != 0)
        {
            std::cerr
                << "BIOS update : Error in libusb_set_configuration -- exit\n";
            libusb_free_device_list(udev->devs, 1);
            goto error_exit;
        }
        std::cerr << "BIOS update : Device is in configured state!\n";
    }

    libusb_free_device_list(udev->devs, 1);

    if (libusb_kernel_driver_active(udev->handle, udev->ci) == 1)
    {
        if (libusb_detach_kernel_driver(udev->handle, udev->ci) != 0)
        {
            std::cerr
                << "BIOS update : Couldn't detach kernel driver -- exit\n";
            libusb_free_device_list(udev->devs, 1);
            goto error_exit;
        }
    }

    ret = libusb_claim_interface(udev->handle, udev->ci);
    if (ret < 0)
    {
        std::cerr << "BIOS update : Couldn't claim interface -- exit. err:"
                  << libusb_error_name(ret) << "\n";
        libusb_free_device_list(udev->devs, 1);
        goto error_exit;
    }

    return 0;
error_exit:
    return -1;
}

int bic_close_usb_dev(usb_dev *udev)
{
    if (libusb_release_interface(udev->handle, udev->ci) < 0)
    {
        std::cerr << "Couldn't release the interface 0x" << std::hex
                  << unsigned(udev->ci) << "\n";
    }

    if (udev->handle != nullptr)
        libusb_close(udev->handle);
    libusb_exit(nullptr);

    return 0;
}

int bic_update_fw_usb(std::string imagePath, usb_dev *udev)
{
    int rc = 0;
    uint8_t *buf = nullptr;
    size_t write_offset = 0;
    size_t file_buf_num_bytes = 0;
    int num_blocks_written = 0;
    int attempts = NUM_ATTEMPTS;

    std::fstream file(imagePath,
                      std::ios::in | std::ios::binary | std::ios::ate);

    if (!file.good())
    {
        std::cout << "BIOS update: fail to open the image.\n";
        return -1;
    }

    file.seekg(0, std::ios::beg);
    buf = new uint8_t[BIOS_UPDATE_BLK_SIZE + USB_PKT_HDR_SIZE];

    uint8_t *file_buf = buf + USB_PKT_HDR_SIZE;
    while (attempts > 0)
    {
        size_t file_buf_pos = 0;
        bool send_packet_fail = false;

        std::cout << "\r" << num_blocks_written << " blocks written...";
        std::cout.flush();

        // Read a block of data from file.
        if (attempts == NUM_ATTEMPTS)
        {
            file.read((char *)file_buf, BIOS_UPDATE_BLK_SIZE);
            file_buf_num_bytes = file.gcount();
            if (file_buf_num_bytes == 0)
            {
                // end of file
                break;
            }
            // Pad to 64K with 0xff, if needed.
            if (file_buf_num_bytes < BIOS_UPDATE_BLK_SIZE)
            {
                std::fill(&buf[file_buf_num_bytes],
                          &buf[file_buf_num_bytes] +
                              (BIOS_UPDATE_BLK_SIZE + USB_PKT_HDR_SIZE -
                               file_buf_num_bytes),
                          0xff);
            }
        }

        while (file_buf_pos < file_buf_num_bytes)
        {
            size_t count = file_buf_num_bytes - file_buf_pos;
            if (count > USB_DAT_SIZE)
                count = USB_DAT_SIZE;

            bic_usb_packet *pkt = reinterpret_cast<bic_usb_packet *>(
                file_buf + file_buf_pos - sizeof(bic_usb_packet));

            pkt->netfn = NETFN_OEM_1S_REQ << 2;
            pkt->cmd = CMD_OEM_1S_UPDATE_FW;
            memcpy(pkt->iana, (uint8_t *)&IANA_ID, IANA_ID_SIZE);
            pkt->target = UPDATE_BIOS;
            pkt->offset = write_offset + file_buf_pos;
            pkt->length = count;
            udev->epaddr = USB_INPUT_PORT;

            rc = send_bic_usb_packet(udev, pkt);
            if (rc < 0)
            {
                fprintf(stderr, "failed to write %zu bytes @ %zu: %d\n", count,
                        write_offset, rc);
                send_packet_fail = true;
                break; // prevent the endless while loop.
            }

            udev->epaddr = USB_OUTPUT_PORT;
            rc = receive_bic_usb_packet(udev, pkt);
            if (rc < 0)
            {
                fprintf(stderr, "Return code : %d\n", rc);
                send_packet_fail = true;
                break; // prevent the endless while loop.
            }

            file_buf_pos += count;
        } // while end

        if (send_packet_fail)
        {
            attempts--;
            continue;
        }

        write_offset += BIOS_UPDATE_BLK_SIZE;
        file_buf_num_bytes = 0;
        num_blocks_written++;
        attempts = NUM_ATTEMPTS;
    } // while

    file.close();
    delete buf;
    if (attempts == 0)
    {
        std::cerr << "\nBIOS update : blocks written failed.\n";
        return -1;
    }

    std::cerr << "\nBIOS update : blocks written done.\n";
    return 0;
}

int update_bic_usb_bios(uint8_t slot_id, [[maybe_unused]] std::string const &imageFilePath)
{
    struct timeval start, end;
    int ret = -1;
    usb_dev bic_udev;
    usb_dev *udev = &bic_udev;

    udev->ci = 1;
    udev->epaddr = USB_INPUT_PORT;

    // init usb device
    ret = bic_init_usb_dev(slot_id, udev, SB_USB_PRODUCT_ID, SB_USB_VENDOR_ID);
    if (ret < 0)
    {
        goto error_exit;
    }

    gettimeofday(&start, nullptr);

    // sending file
    ret = bic_update_fw_usb(imageFilePath, udev);

    if (ret < 0)
        goto error_exit;

    gettimeofday(&end, nullptr);
    std::cerr << "Elapsed time: " << (int)(end.tv_sec - start.tv_sec)
              << "sec.\n";

    ret = 0;

error_exit:
    // close usb device
    bic_close_usb_dev(udev);

    return ret;
}