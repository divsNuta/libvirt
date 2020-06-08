/*
 * sysinfotest.c: Testcase(s) for virSysinfoRead
 *
 * Copyright (C) 2013 Red Hat, Inc.
 * Copyright IBM Corp. 2012
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
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <unistd.h>

#include "internal.h"

#include "virbuffer.h"
#include "virsysinfo.h"
#include "testutils.h"
#include "virfile.h"
#include "virstring.h"

#define LIBVIRT_VIRSYSINFOPRIV_H_ALLOW
#include "virsysinfopriv.h"

#define LIBVIRT_VIRCOMMANDPRIV_H_ALLOW
#include "vircommandpriv.h"

#define VIR_FROM_THIS VIR_FROM_NONE

struct testSysinfoData {
    const char *name; /* test name, also base name for result files */
    virSysinfoDefPtr (*func)(void); /* sysinfo gathering function */
};


static void
testDMIDecodeDryRun(const char *const*args G_GNUC_UNUSED,
                    const char *const*env G_GNUC_UNUSED,
                    const char *input G_GNUC_UNUSED,
                    char **output,
                    char **error,
                    int *status,
                    void *opaque)
{
    const char *sysinfo = opaque;

    if (virFileReadAll(sysinfo, 10 * 1024 * 1024, output) < 0) {
        *error = g_strdup(virGetLastErrorMessage());
        *status = EXIT_FAILURE;
        return;
    }

    *error = g_strdup("");
    *status = 0;
}


static int
testSysinfo(const void *data)
{
    const struct testSysinfoData *testdata = data;
    const char *sysfsActualData;
    g_auto(virSysinfoDefPtr) ret = NULL;
    g_auto(virBuffer) buf = VIR_BUFFER_INITIALIZER;
    g_autofree char *sysinfo = NULL;
    g_autofree char *cpuinfo = NULL;
    g_autofree char *expected = NULL;

    sysinfo = g_strdup_printf("%s/sysinfodata/%ssysinfo.data", abs_srcdir, testdata->name);
    cpuinfo = g_strdup_printf("%s/sysinfodata/%scpuinfo.data", abs_srcdir, testdata->name);
    expected = g_strdup_printf("%s/sysinfodata/%ssysinfo.expect", abs_srcdir, testdata->name);

    virCommandSetDryRun(NULL, testDMIDecodeDryRun, sysinfo);

    virSysinfoSetup(NULL, sysinfo, cpuinfo);

    ret = testdata->func();
    virCommandSetDryRun(NULL, NULL, NULL);

    if (!ret)
        return -1;

    if (virSysinfoFormat(&buf, ret) < 0)
        return -1;

    if (!(sysfsActualData = virBufferCurrentContent(&buf)))
        return -1;

    return virTestCompareToFile(sysfsActualData, expected);
}


#define TEST(name, func) \
    do { \
        struct testSysinfoData data = { name, func }; \
        if (virTestRun(name " sysinfo", testSysinfo, &data) < 0) \
            ret = EXIT_FAILURE; \
    } while (0)

static int
mymain(void)
{
    int ret = EXIT_SUCCESS;

    TEST("s390", virSysinfoReadS390);
    TEST("s390-freq", virSysinfoReadS390);
    TEST("ppc", virSysinfoReadPPC);
    TEST("x86", virSysinfoReadDMI);
    TEST("arm", virSysinfoReadARM);
    TEST("arm-rpi2", virSysinfoReadARM);
    TEST("aarch64", virSysinfoReadARM);
    TEST("aarch64-moonshot", virSysinfoReadARM);
    TEST("aarch64-gigabyte", virSysinfoReadARM);

    return ret;
}

#undef TEST
#undef TEST_FULL

VIR_TEST_MAIN(mymain)
