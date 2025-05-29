3. Éviter les appels système inutiles en mode monitor
La fonction display_monitor_status() fait beaucoup d'appels printf. Vous pourriez optimiser :
Ajoutez cette fonction optimisée :

void display_monitor_status_optimized(quadrature_state_t *state) {
    if (!monitor_mode) return;
    
    static char buffer[2048];  // Buffer statique pour éviter les allocations
    int pos = 0;
    
    // Construire tout l'affichage dans le buffer
    pos += sprintf(buffer + pos, SAVE_CURSOR CURSOR_HOME);
    pos += sprintf(buffer + pos, "\033[36m═══════════════════════════════════════════════════════════════════════════════\n\033[0m");
    pos += sprintf(buffer + pos, "\033[36m                           ATARI ST MOUSE SIMULATOR                            \n\033[0m");
    pos += sprintf(buffer + pos, "\033[36m═══════════════════════════════════════════════════════════════════════════════\n\033[0m");
    // ... continuer pour tout l'affichage
    
    // Un seul appel write() au lieu de multiples printf()
    write(STDOUT_FILENO, buffer, pos);
}


4. Optimisation de la fréquence d'affichage du monitor
Ajoutez un throttling pour l'affichage :
Ajoutez ces variables globales :



static struct timespec last_display_time = {0, 0};
#define DISPLAY_INTERVAL_MS 50  // 20 FPS maximum



Ajoutez cette fonction :



int should_update_display() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    long elapsed_ms = (now.tv_sec - last_display_time.tv_sec) * 1000 + 
                      (now.tv_nsec - last_display_time.tv_nsec) / 1000000;
    
    if (elapsed_ms >= DISPLAY_INTERVAL_MS) {
        last_display_time = now;
        return 1;
    }
    return 0;
}



Modifiez les appels à display_monitor_status :
Remplacez :

if (monitor_mode) {
    display_monitor_status(state);
}

par

if (monitor_mode && should_update_display()) {
    display_monitor_status(state);
}