import numpy as np
import sys
import os

def compare_ds2_universal(file1, file2, type_arg="double", chunk_size=1024*1024):
    try:
        # 1. Configurazione del Tipo di Dato
        type_arg = type_arg.lower()
        
        if type_arg == "float":
            current_dtype = np.float32
            type_label = "FLOAT (32-bit)"
            is_integer = False
        elif type_arg == "double":
            current_dtype = np.float64
            type_label = "DOUBLE (64-bit)"
            is_integer = False
        elif type_arg == "int":
            current_dtype = np.int32
            type_label = "INT (32-bit)"
            is_integer = True
        else:
            print(f"Errore: Tipo '{type_arg}' non riconosciuto. Usa 'float', 'double' o 'int'.")
            return

        f1_size = os.path.getsize(file1)
        f2_size = os.path.getsize(file2)
        
        if f1_size != f2_size:
            print(f"ERRORE DIMENSIONI FILE: {f1_size} vs {f2_size} bytes")
            return

        with open(file1, 'rb') as f1, open(file2, 'rb') as f2:
            # L'header è SEMPRE int32 (4 byte), indipendentemente dai dati
            h1 = np.fromfile(f1, dtype=np.int32, count=2)
            h2 = np.fromfile(f2, dtype=np.int32, count=2)
            
            if not np.array_equal(h1, h2):
                print(f"ERRORE HEADER (Righe x Colonne): {h1} vs {h2}")
                return

            print(f"Analisi dataset {h1[0]}x{h1[1]} in modalità {type_label}...")
            
            max_diff = 0.0
            val_mine_at_max = 0.0
            val_ref_at_max = 0.0
            
            sum_diff = 0.0
            sum_pct_error = 0.0
            total_elements = 0
            nonzero_elements = 0
            
            while True:
                # 2. Lettura dinamica usando current_dtype
                d1 = np.fromfile(f1, dtype=current_dtype, count=chunk_size)
                d2 = np.fromfile(f2, dtype=current_dtype, count=chunk_size)
                
                if d1.size == 0:
                    break
                
                # Conversione in float64 per calcoli statistici (evita overflow anche per gli int)
                d1_calc = d1.astype(np.float64)
                d2_calc = d2.astype(np.float64)

                diff = np.abs(d1_calc - d2_calc)
                
                current_max = np.max(diff)
                if current_max > max_diff:
                    max_diff = current_max
                    idx = np.argmax(diff)
                    val_mine_at_max = d1_calc[idx]
                    val_ref_at_max = d2_calc[idx]
                
                sum_diff += np.sum(diff)
                total_elements += d1.size

                refs = np.abs(d2_calc)
                # Evitiamo divisione per zero
                mask = refs > 1e-20
                
                if np.any(mask):
                    pcts = (diff[mask] / refs[mask])
                    sum_pct_error += np.sum(pcts)
                    nonzero_elements += np.count_nonzero(mask)

            mae = sum_diff / total_elements
            mape = (sum_pct_error / nonzero_elements) * 100 if nonzero_elements > 0 else 0.0

            print("-" * 50)
            print("DETTAGLIO MASSIMA DIFFERENZA TROVATA:")
            # Se è int, stampiamo senza decimali inutili
            if is_integer:
                print(f"Max Diff Assoluta:        {int(max_diff)}")
                print(f"Valore Tuo:               {int(val_mine_at_max)}")
                print(f"Valore Prof:              {int(val_ref_at_max)}")
            else:
                print(f"Max Diff Assoluta:        {max_diff:.20f}")
                print(f"Valore Tuo:               {val_mine_at_max:.20f}")
                print(f"Valore Prof:              {val_ref_at_max:.20f}")
                
            print("-" * 50)
            print("STATISTICHE GLOBALI (ERRORE MEDIO):")
            print(f"Errore Medio Assoluto (MAE):    {mae:.10f}")
            print(f"Errore Medio Percentuale (MAPE): {mape:.10f}%")
            print("-" * 50)
            
            # 3. Logica di successo differenziata per Interi vs Float/Double
            if is_integer:
                if max_diff == 0:
                     print("✅ RISULTATO: ECCELLENTE (File Identici)")
                else:
                     print("❌ RISULTATO: ERRORE (Gli interi devono essere esatti)")
            else:
                # Soglie per Float/Double
                threshold = 1e-10 if current_dtype == np.float64 else 1e-5
                
                if mape < threshold:
                     print("✅ RISULTATO: ECCELLENTE (Matematicamente Identici)")
                elif mape < (threshold * 10000):
                     print("✅ RISULTATO: OTTIMO (Differenze floating point normali)")
                else:
                     print("⚠️ RISULTATO: ATTENZIONE (Possibile errore logico)")

    except Exception as e:
        print(f"Errore: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Uso: python3 check.py <file_nostro> <file_prof> [double|float|int]")
        print("Default: double")
    else:
        # Gestione del terzo argomento opzionale
        tipo = "double"
        if len(sys.argv) > 3:
            tipo = sys.argv[3]
        
        compare_ds2_universal(sys.argv[1], sys.argv[2], tipo)
