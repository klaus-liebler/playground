def berechne_bmi(m_in_Kg, l_in_m):
    l_squared=l_in_m*l_in_m
    ergebnis=m_in_Kg/l_squared
    return ergebnis
y=berechne_bmi(80, 1.86)
print(y)

