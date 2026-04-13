#any startup process or shell get zsys env before run source env commands

   printer "###PRINTENV with source /etc/profile.d/zsys*.sh------------------- ###"
    for file in /etc/profile.d/zsys*; do
        if [ -f "$file" ]; then
            echo "Sourcing $file"
            . "$file"
        fi
    done

