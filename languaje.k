for (Init int i <- 0; i < max; i = i + 1) {
    if (registry.get_user(i).is_active =? true) {
        log_activity("User_" + i, true);
    }
}