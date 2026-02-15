#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

#include <soc/oplus/oplus_project.h>
#include <soc/oplus/oplus_project_oldcdt.h>

#include <soc/qcom/smem.h>

#define SMEM_PROJECT 135

static struct proc_dir_entry *oppoVersion = NULL;
static ProjectInfoCDTType_oldcdt *format = NULL;

static const char* nfc_feature = "nfc_feature";
static const char* feature_src = "/vendor/etc/nfc/com.oppo.nfc_feature.xml";

void init_project_version(void)
{
    void* smem_addr;
    unsigned int smem_size;
    
    if (format) {
        return;
    }
    else {
        smem_addr = smem_get_entry(SMEM_PROJECT, &smem_size, 0, SMEM_ANY_HOST_FLAG);
        if (IS_ERR(smem_addr)) {
            pr_err("unable to acquire smem SMEM_PROJECT entry\n");
            return;
        }
        
        format = (ProjectInfoCDTType_oldcdt *)smem_addr;
        if (format == ERR_PTR(-EPROBE_DEFER)) {
            format = NULL;
            return;
        }
        
        pr_info("KE Project:%d, nRF:%d, PCB:%d\n", format->nproject, format->nmodem, format->npcbversion);
        pr_info("OCP: %d %d %d %d\n", format->npmicocp[0], format->npmicocp[1], format->npmicocp[2], format->npmicocp[3]);
    }
    pr_info("get_project:%d, is_new_cdt:%d, get_PCB_Version:%d\n", get_project(), is_new_cdt(), get_PCB_Version());
    pr_info("get_Oppo_Boot_Mode:%d, get_Modem_Version:%d, get_Operator_Version:%d\n", get_Oppo_Boot_Mode(), get_Modem_Version(), get_Operator_Version());
    pr_info("oppo project info loading finished\n");
}

unsigned int get_project(void)
{
    init_project_version();

    return format ? format->nproject : 0;
}
EXPORT_SYMBOL(get_project);

unsigned int is_project(int project)
{
    init_project_version();

    return (get_project() == project? 1 : 0 );
}
EXPORT_SYMBOL(is_project);

unsigned int is_new_cdt(void)
{
    return 0;
}
EXPORT_SYMBOL(is_new_cdt);

unsigned char get_PCB_Version(void)
{
    init_project_version();

    return format ? format->npcbversion : -EINVAL;
}
EXPORT_SYMBOL(get_PCB_Version);

unsigned char get_Oppo_Boot_Mode(void)
{
    init_project_version();

    return format ? format->noppobootmode : 0;
}
EXPORT_SYMBOL(get_Oppo_Boot_Mode);

unsigned char get_Modem_Version(void)
{
    init_project_version();

    return format ? format->nmodem : -EINVAL;
}
EXPORT_SYMBOL(get_Modem_Version);

unsigned char get_Operator_Version(void)
{
    init_project_version();

    return format ? format->noperator : -EINVAL;
}
EXPORT_SYMBOL(get_Operator_Version);

extern char* saved_command_line;


static int oppo_eng_version = OPPO_ENG_VERSION_NOT_INIT;
static int oppo_confidential = true;

int get_eng_version(void)
{
    int i = 0, eng_len = 3;
    char *substr = NULL;

    if (oppo_eng_version != OPPO_ENG_VERSION_NOT_INIT)
        return oppo_eng_version;

    if (strstr(boot_command_line, "is_confidential=0"))
        oppo_confidential = false;

    oppo_eng_version = 0;
    substr = strstr(boot_command_line, "eng_version=");
    if (!substr) {      //if cmdline does't cover the version, we use normal version as default version
        printk(KERN_EMERG "cmdline does't have the sw_version %s \n", __func__);
        return oppo_eng_version;
    }

    substr += strlen("eng_version=");
    for (i = 0; substr[i] != ' ' && i < eng_len && substr[i] != '\0'; i++) {
        if ((substr[i] >= '0') && (substr[i] <= '9')) {
            oppo_eng_version = oppo_eng_version * 10 + substr[i] - '0';
        } else {
            oppo_eng_version = 0;
            break;
        }
    }

    return oppo_eng_version;
}
EXPORT_SYMBOL(get_eng_version);

bool is_confidential(void)
{
    if (oppo_eng_version != OPPO_ENG_VERSION_NOT_INIT)
        return oppo_confidential;

    get_eng_version();

    return oppo_confidential;
}
EXPORT_SYMBOL(is_confidential);

bool oppo_daily_build(void) {
    static int daily_build = -1;
    int eng_version = 0;

    if (daily_build != -1) return daily_build;

    if (strstr(saved_command_line, "buildvariant=userdebug") ||
        strstr(saved_command_line, "buildvariant=eng")) {
        daily_build = true;
    } else {
        daily_build = false;
    }

    eng_version = get_eng_version();
    if ((ALL_NET_CMCC_TEST == eng_version) || (ALL_NET_CMCC_FIELD == eng_version) ||
        (ALL_NET_CU_TEST == eng_version) || (ALL_NET_CU_FIELD == eng_version) ||
        (ALL_NET_CT_TEST == eng_version) || (ALL_NET_CT_FIELD == eng_version)) {
        daily_build = false;
    }

    return daily_build;
}
EXPORT_SYMBOL(oppo_daily_build);

/*this module just init for creat files to show which version*/
static ssize_t prjVersion_read_proc(struct file *file, char __user *buf,
                size_t count, loff_t *off)
{
    char page[256] = {0};
    int len = 0;
    len = sprintf(page, "%d", get_project());

    if (len > *off) {
        len -= *off;
    }
    else
        len = 0;
    if (copy_to_user(buf, page, (len < count ? len : count))) {
        return -EFAULT;
    }
    *off += len < count ? len : count;
    return (len < count ? len : count);
}

struct file_operations prjVersion_proc_fops = {
    .read = prjVersion_read_proc,
    .write = NULL,
};

static ssize_t pcbVersion_read_proc(struct file *file, char __user *buf,
                size_t count, loff_t *off)
{
    char page[256] = {0};
    int len = 0;

    len = sprintf(page, "%d", get_PCB_Version());

    if (len > *off) {
        len -= *off;
    }
    else
        len = 0;

    if (copy_to_user(buf, page, (len < count ? len : count))) {
        return -EFAULT;
    }
    *off += len < count ? len : count;
    return (len < count ? len : count);
}

struct file_operations pcbVersion_proc_fops = {
    .read = pcbVersion_read_proc,
};


static ssize_t operatorName_read_proc(struct file *file, char __user *buf,
                size_t count, loff_t *off)
{
    char page[256] = {0};
    int len = 0;

    len = sprintf(page, "%d", get_Operator_Version());

    if (len > *off) {
        len -= *off;
    }
    else
        len = 0;

    if (copy_to_user(buf, page, (len < count ? len : count))) {
        return -EFAULT;
    }
    *off += len < count ? len : count;
    return (len < count ? len : count);
}

struct file_operations operatorName_proc_fops = {
    .read = operatorName_read_proc,
};

static ssize_t modemType_read_proc(struct file *file, char __user *buf,
                size_t count, loff_t *off)
{
    char page[256] = {0};
    int len = 0;

    len = sprintf(page, "%d", get_Modem_Version());

    if (len > *off) {
        len -= *off;
    }
    else
        len = 0;

    if (copy_to_user(buf, page, (len < count ? len : count))) {
        return -EFAULT;
    }
    *off += len < count ? len : count;
    return (len < count ? len : count);
}

struct file_operations modemType_proc_fops = {
    .read = modemType_read_proc,
};

static ssize_t oppoBootmode_read_proc(struct file *file, char __user *buf,
                size_t count, loff_t *off)
{
    char page[256] = {0};
    int len = 0;

    len = sprintf(page, "%d", get_Oppo_Boot_Mode());

    if (len > *off) {
        len -= *off;
    }
    else
        len = 0;

    if (copy_to_user(buf, page, (len < count ? len : count))) {
        return -EFAULT;
    }
    *off += len < count ? len : count;
    return (len < count ? len : count);
}

struct file_operations oppoBootmode_proc_fops = {
    .read = oppoBootmode_read_proc,
};

#define OEM_SEC_BOOT_REG 0x780350 /*sdm660
*/
static ssize_t secureType_read_proc(struct file *file, char __user *buf,
                size_t count, loff_t *off)
{
    char page[256] = {0};
    int len = 0;
    void __iomem *oem_config_base;
    uint32_t secure_oem_config = 0;

    oem_config_base = ioremap(OEM_SEC_BOOT_REG, 4);
    if (!oem_config_base) {
        pr_err("SecureType: ioremap failed\n");
        return -EFAULT;
    }
    secure_oem_config = __raw_readl(oem_config_base);
    iounmap(oem_config_base);
    printk(KERN_EMERG "lycan test secure_oem_config 0x%x\n", secure_oem_config);
    len = sprintf(page, "%d", secure_oem_config);

    if (len > *off) {
        len -= *off;
    }
    else
        len = 0;

    if (copy_to_user(buf, page, (len < count ? len : count))) {
        return -EFAULT;
    }
    *off += len < count ? len : count;
    return (len < count ? len : count);
}

struct file_operations secureType_proc_fops = {
    .read = secureType_read_proc,
};

#define QFPROM_RAW_SERIAL_NUM 0x00786134 /*different at each platform, please ref boot_images\core\systemdrivers\hwio\scripts\xxx\hwioreg.per
*/

static unsigned int g_serial_id = 0x00; /*maybe can use for debug
*/

static ssize_t serialID_read_proc(struct file *file, char __user *buf,
                size_t count, loff_t *off)
{
    char page[256] = {0};
    int len = 0;
    len = sprintf(page, "0x%x", g_serial_id);

    if (len > *off) {
        len -= *off;
    }
    else
        len = 0;

    if (copy_to_user(buf, page, (len < count ? len : count))) {
        return -EFAULT;
    }
    *off += len < count ? len : count;
    return (len < count ? len : count);
}


struct file_operations serialID_proc_fops = {
    .read = serialID_read_proc,
};

static ssize_t ocplog_read_proc(struct file *file, char __user *buf,
                size_t count, loff_t *off)
{
    char page[256] = {0};
    int len = 0;
    int i = 0;

    init_project_version();

    if (format) {
        len += sprintf(&page[len], "ocp:");
        for (i = 0;i < OCPCOUNTMAX;i++) {
            len += sprintf(&page[len], " %d", format->npmicocp[i]);
        }
        len += sprintf(&page[len], "\n");
    }
    else
        len += sprintf(&page[len], "Failed to get ocp info\n");

    if (len > *off) {
        len -= *off;
    }
    else
        len = 0;

    if (copy_to_user(buf, page, (len < count ? len : count))) {
        return -EFAULT;
    }
    *off += len < count ? len : count;
    return (len < count ? len : count);
}


struct file_operations ocp_proc_fops = {
    .read = ocplog_read_proc,
};


static ssize_t eng_version_read_proc(struct file *file, char __user *buf,
                size_t count, loff_t *off)
{
    int ret = 0;
    char page[64] = {0};

    get_eng_version();
    snprintf(page, 63, "%d", oppo_eng_version);
    ret = simple_read_from_buffer(buf, count, off, page, strlen(page));

    return ret;
}

struct file_operations eng_version_proc_fops = {
    .read = eng_version_read_proc,
    .open  = simple_open,
    .owner = THIS_MODULE,
};

static ssize_t confidential_read_proc(struct file *file, char __user *buf,
                size_t count, loff_t *off)
{
    int ret = 0;
    char page[64] = {0};

    snprintf(page, 63, "%d", is_confidential());
    ret = simple_read_from_buffer(buf, count, off, page, strlen(page));

    return ret;
}

struct file_operations confidential_proc_fops = {
    .read = confidential_read_proc,
    .open  = simple_open,
    .owner = THIS_MODULE,
};

static int __init oppo_project_init(void)
{
    int ret = 0;
    struct proc_dir_entry *pentry;
    void __iomem *serial_id_addr = NULL;

    serial_id_addr = ioremap(QFPROM_RAW_SERIAL_NUM , 4);
    if (serial_id_addr) {
        g_serial_id = __raw_readl(serial_id_addr);
        iounmap(serial_id_addr);
        printk(KERN_EMERG "serialID 0x%x\n", g_serial_id);
    } else
    {
        g_serial_id = 0xffffffff;
    }

    get_eng_version();

    oppoVersion =  proc_mkdir("oppoVersion", NULL);
    if (!oppoVersion) {
        pr_err("can't create oppoVersion proc\n");
        goto ERROR_INIT_VERSION;
    }
    pentry = proc_create("prjName", S_IRUGO, oppoVersion, &prjVersion_proc_fops);
    if (!pentry) {
        pr_err("create prjVersion proc failed.\n");
        goto ERROR_INIT_VERSION;
    }
    pentry = proc_create("pcbVersion", S_IRUGO, oppoVersion, &pcbVersion_proc_fops);
    if (!pentry) {
        pr_err("create pcbVersion proc failed.\n");
        goto ERROR_INIT_VERSION;
    }
    pentry = proc_create("operatorName", S_IRUGO, oppoVersion, &operatorName_proc_fops);
    if (!pentry) {
        pr_err("create operatorName proc failed.\n");
        goto ERROR_INIT_VERSION;
    }
    pentry = proc_create("modemType", S_IRUGO, oppoVersion, &modemType_proc_fops);
    if (!pentry) {
        pr_err("create modemType proc failed.\n");
        goto ERROR_INIT_VERSION;
    }
    pentry = proc_create("oppoBootmode", S_IRUGO, oppoVersion, &oppoBootmode_proc_fops);
    if (!pentry) {
        pr_err("create oppoBootmode proc failed.\n");
        goto ERROR_INIT_VERSION;
    }
    pentry = proc_create("secureType", S_IRUGO, oppoVersion, &secureType_proc_fops);
    if (!pentry) {
        pr_err("create secureType proc failed.\n");
        goto ERROR_INIT_VERSION;
    }

    pentry = proc_create("serialID", S_IRUGO, oppoVersion, &serialID_proc_fops);
    if (!pentry) {
        pr_err("create serialID proc failed.\n");
        goto ERROR_INIT_VERSION;
    }

    pentry = proc_create("ocp", S_IRUGO, oppoVersion, &ocp_proc_fops);
    if (!pentry) {
        pr_err("create serialID proc failed.\n");
        goto ERROR_INIT_VERSION;
    }

    pentry = proc_create("engVersion", S_IRUGO, oppoVersion, &eng_version_proc_fops);
    if (!pentry) {
        pr_err("create engVersion proc failed.\n");
        goto ERROR_INIT_VERSION;
    }

    pentry = proc_create("isConfidential", S_IRUGO, oppoVersion, &confidential_proc_fops);
    if (!pentry) {
        pr_err("create isConfidential proc failed.\n");
        goto ERROR_INIT_VERSION;
    }

    return ret;
ERROR_INIT_VERSION:
        remove_proc_entry("oppoVersion", NULL);
        return -ENOENT;
}
arch_initcall(oppo_project_init);