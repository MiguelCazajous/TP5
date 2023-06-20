#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#define BUFFER_LENGTH PAGE_SIZE
#define MAX_RETRIES 5
#define SLEEP_INTERVAL 1000

// Pines entrada GPIO
#define GPIO_IN_1 1
#define GPIO_IN_2 2
#define GPIO_IN_3 3
#define GPIO_IN_4 4
#define GPIO_IN_5 5
#define GPIO_IN_6 6

static struct proc_dir_entry *proc_entry;
// Variable usada en operaciones read y write.
static char *shared_file;

static ssize_t shared_file_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
    int available_space = BUFFER_LENGTH-1;

    printk(KERN_INFO "shared_file_write()\n");
    
    if ((*off) > 0) 
      return 0;
    
    if (len > available_space) {
      printk(KERN_INFO "shared_file: No hay suficiente espacio!!\n");
      return -ENOSPC;
    }
    
    /* Transfiere data desde el espacio de usuario al espacio de kernel */
    /* echo un mensaje > /proc/shared_file                                */
    if (copy_from_user(&shared_file[0], buf, len))
      return -EFAULT;

    shared_file[len] = '\0';  /* Caracter de terminación '\0' */  
    *off+=len;              /* Actualiza puntero */

    printk(KERN_INFO "Mensaje enviado: %s", shared_file);
    
    return len;
}


static ssize_t shared_file_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    int nr_bytes;
    int result = 0;
    
    printk(KERN_INFO "shared_file_read()\n");
    
    if ((*off) > 0) {/*No hay nada más para leer.*/
	printk(KERN_INFO "Nada para leer\n");
        return 0;
    }
      
    nr_bytes=strlen(shared_file);

    printk(KERN_INFO "Valor leído %s", shared_file);

    //Backup de shared_file para almacenar el valor que se envía al espacio de usuario.
    char *temp = (char*)vmalloc(BUFFER_LENGTH);
    strncpy(temp, shared_file, nr_bytes);
      
    if (len<nr_bytes) {
	vfree(temp);
        return -ENOSPC;
    }

    if (strncmp(shared_file, "1", 1) == 0) {
        result = gpio_get_value(GPIO_IN_1)+gpio_get_value(GPIO_IN_2)*2+gpio_get_value(GPIO_IN_3)*3;
        printk(KERN_INFO "Sensor 1: pines 1, 2 y 3. Resultado: %d\n", result);
	memset(shared_file, 0, BUFFER_LENGTH);
    	snprintf(shared_file, BUFFER_LENGTH, "Sensor 1: %d", result);
    } else if (strncmp(shared_file, "2", 1) == 0) {
        result = gpio_get_value(GPIO_IN_4)*4+gpio_get_value(GPIO_IN_5)*5+gpio_get_value(GPIO_IN_6)*6;
        printk(KERN_INFO "Sensor 2: pines 4, 5 y 6. Resultado: %d\n", result);
	memset(shared_file, 0, BUFFER_LENGTH);
    	snprintf(shared_file, BUFFER_LENGTH, "Sensor 2: %d", result);
    } else {
        printk(KERN_INFO "Opción inválida: %s\n", shared_file);
	memset(shared_file, 0, BUFFER_LENGTH);
        snprintf(shared_file, BUFFER_LENGTH, "Opción inválida");
    }

    
    /* Transfiere data desde el espacio de kernel al espacio de usuario */ 
    /* cat /proc/shared_file                                           */
    if (copy_to_user(buf, &shared_file[0], nr_bytes)) {
	vfree(temp);
        return -EINVAL;
    }

    // Restauro valor
    strncpy(shared_file, temp, strlen(temp));

    (*off)+=len;  /* Actualiza puntero */

    vfree(temp);
    return nr_bytes; 
}

static const struct proc_ops proc_entry_fops = {
    .proc_read = shared_file_read,
    .proc_write = shared_file_write,    
};

void gpio_initializer(const int gpio_n, const char* label) {
    int err = 0;

    if (gpio_is_valid(gpio_n) == false) {
        printk(KERN_INFO "GPIO PIN: %d es inválido", gpio_n);
        gpio_free(gpio_n);
    }

    if ((err = gpio_request(gpio_n, label)), err < 0) {
	printk(KERN_INFO "No se pudo solicitar GPIO pin %d. Error %d\n", gpio_n, err);
	gpio_free(gpio_n);
    }
  
    if(gpio_direction_input(gpio_n) != 0){
        printk(KERN_INFO "No se pudo configurar GPIO pin %d.\n", gpio_n);
        gpio_free(gpio_n);
    }
}

int init_GPIO_module( void )
{
    int ret = 0;
    shared_file = (char *)vmalloc(BUFFER_LENGTH);

    if (!shared_file) {
        ret = -ENOMEM;
    } else {
        memset(shared_file, 0, BUFFER_LENGTH);
	strncpy(shared_file, "1", 1);
	printk(KERN_INFO "Inicializamos el sensor por defecto: %s", shared_file);
        proc_entry = proc_create("shared_file", 0666, NULL, &proc_entry_fops);
        if (proc_entry == NULL) {
            ret = -ENOMEM;
            vfree(shared_file);
            printk(KERN_INFO "GPIO_module: No puede crear entrada en /proc..!!\n");
        } else {
            printk(KERN_INFO "GPIO_module: Modulo cargado..!!\n");
        }
    }

    // Inicializamos los pines a usar
    gpio_initializer(GPIO_IN_1, "GPIO_IN_1");
    gpio_initializer(GPIO_IN_2, "GPIO_IN_2");
    gpio_initializer(GPIO_IN_3, "GPIO_IN_3");
    gpio_initializer(GPIO_IN_4, "GPIO_IN_4");
    gpio_initializer(GPIO_IN_5, "GPIO_IN_5");
    gpio_initializer(GPIO_IN_6, "GPIO_IN_6");
    return ret;
}

void exit_GPIO_module( void )
{
    remove_proc_entry("shared_file", NULL);
    vfree(shared_file);
    printk(KERN_INFO "GPIO_module: Modulo descargado..!!\n");
}

module_init(init_GPIO_module);
module_exit(exit_GPIO_module);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Módulo de lectura y procesamiento de entradas GPIO.");
MODULE_AUTHOR("Grupo NullPointer");

