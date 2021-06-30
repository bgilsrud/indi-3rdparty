
struct ft_model {
    const char *name; /* The camera model name */
    uint16_t vendor_id;  /* The USB vendor ID */
    uint16_t product_id;  /* The USB product ID */
    struct ft_sensor *sensor; /* The sensor that the camera has */
};

static const struct ft_camera supported_cameras[] = {
    {
        .name = "Meade LPI-GM",
        .vendor_id = 0xdead,
        .vendor_id = 0xbeef,
        .sensor = NULL
    },
    {0}
};
