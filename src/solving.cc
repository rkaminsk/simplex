#include <problem.hh>

// TODO: port to C++

/*
@dataclass
class Tableau:
    '''
    A sparse matrix with lazy deletion.

    Note: In C++ I would probably implement this with two ordered maps to get a
    more compact and easier to understand implementation. The interface would
    be roughly the same though.
    '''
    _vals: Dict[Tuple[int, int], Tuple[Fraction, int]] = field(default_factory = dict)
    _rows: List[List[int]] = field(default_factory = list)
    _cols: List[List[int]] = field(default_factory = list)

    def _reserve_row(self, i):
        while len(self._rows) <= i:
            self._rows.append([])

    def _reserve_col(self, j):
        while len(self._cols) <= j:
            self._cols.append([])

    def get(self, i: int, j: int) -> Number:
        '''
        Get the value at row i and column j.
        '''
        return self._vals.get((i,j), (Fraction(0), 0))[0]

    def set(self, i: int, j: int, v: Number) -> None:
        '''
        Set value v at row i and column j.
        '''
        old = self._vals.get((i, j))
        if v == 0:
            if old is not None and old[0] != 0:
                self._vals[(i, j)] = (Fraction(0), 3)
        else:
            if old is None:
                self._reserve_row(i)
                self._rows[i].append(j)
                self._reserve_col(j)
                self._cols[j].append(i)
            elif old[0] == 0:
                if old[1] & 1 == 0:
                    self._rows[i].append(j)
                if old[1] & 2 == 0:
                    self._cols[j].append(i)
            self._vals[(i, j)] = (v, 3)

    def row(self, i: int) -> Iterator[Tuple[int, Number]]:
        '''
        Return column indices associated with non-zero values.
        '''
        self._reserve_row(i)
        l = 0
        row = self._rows[i]
        for k, j in enumerate(row):
            v, s = self._vals[(i, j)]
            if v == 0:
                if s & 1 == 1:
                    s = s ^ 1
                if s == 0:
                    del self._vals[(i, j)]
                else:
                    self._vals[(i, j)] = (v, s)
            else:
                if l != k:
                    row[l] = row[k]
                l += 1
                yield j, v
        del row[l:]

    def col(self, j: int) -> Iterator[Tuple[int, Number]]:
        '''
        Return row indices associated with non-zero values.
        '''
        self._reserve_col(j)
        l = 0
        col = self._cols[j]
        for k, i in enumerate(col):
            v, s = self._vals[(i, j)]
            if v == 0:
                if s & 2 == 2:
                    s = s ^ 2
                if s == 0:
                    del self._vals[(i, j)]
                else:
                    self._vals[(i, j)] = (v, s)
            else:
                if l != k:
                    col[l] = col[k]
                l += 1
                yield i, v
        del col[l:]

    def n_rows(self):
        '''
        Return the number of rows in the tableau.
        '''
        return len(self._rows)


@dataclass
class Solver:
    '''
    A problem in form of a set of equations that can be checked for
    satisfiability.
    '''
    equations: List[Equation]
    tableau: Tableau = field(default_factory=Tableau)
    lower: Mapping[int, Number] = field(default_factory=dict)
    upper: Mapping[int, Number] = field(default_factory=dict)
    assignment: List[Number] = field(default_factory=list)
    variables: List[int] = field(default_factory=list)
    n_basic: int = 0
    n_pivots: int = 0

    def vars(self) -> List[str]:
        '''
        The set of all variables (in form of a list) appearing in the
        equations.
        '''
        return sorted(set(chain(
            chain.from_iterable(x.vars() for x in self.equations))))

    def check(self) -> bool:
        '''
        Check if the state invariants hold.
        '''
        for i in range(self.tableau.n_rows()):
            v_i = 0
            for j, a_ij in self.tableau.row(i):
                v_i += self.assignment[j] * a_ij

            if v_i != self.assignment[self.n_basic + i]:
                return False

        return True

    def pivot(self, i: int, j: int, v: Number) -> None:
        '''
        Pivots basic variable x_i and non-basic variable x_j.
        '''
        a_ij = self.tableau.get(i, j)
        assert a_ij != 0

        # adjust assignment
        ii = i + self.n_basic
        dj = (v - self.assignment[ii]) / a_ij
        self.assignment[ii] = self.assignment[j] + dj
        self.assignment[j] = v

        # invert row i
        for k, a_ik in self.tableau.row(i):
            if k == j:
                self.tableau.set(i, k, 1 / a_ij)
            else:
                self.tableau.set(i, k, a_ik / -a_ij)

        # eliminate x_j from rows k != i
        for k, a_kj in self.tableau.col(j):
            if k == i:
                continue
            for l, a_il in self.tableau.row(i):
                if l == j:
                    a_kl = a_kj / a_ij
                else:
                    a_kl = self.tableau.get(k, l) + a_il * a_kj
                self.tableau.set(k, l, a_kl)
            # TODO:
            # at this point we can already check if the basic variable x_k is conflicting
            # and we can identify the best non-basic variable for pivoting
            # for not having to iterate in the select function
            # most notably we can already check if the problem is unsatisfiable here
            # if x_k is conflicting but cannot be adjusted, then the problem is unsatisfiable
            v_k = Number(0)
            for l, a_kl in self.tableau.row(k):
                v_k += a_kl * self.assignment[l]
            self.assignment[self.n_basic + k] = v_k

        # swap variables x_i and x_j
        self.variables[ii], self.variables[j] = self.variables[j], self.variables[ii]

        self.n_pivots += 1
        assert self.check()

    def select(self) -> Union[bool, Tuple[int, int, Number]]:
        '''
        Select pivot point using Bland's rule.
        '''
        basic = sorted(enumerate(self.variables[self.n_basic:]), key=lambda x: x[1])
        nonbasic = sorted(enumerate(self.variables[:self.n_basic]), key=lambda x: x[1])
        for i, xi in basic:
            axi = self.assignment[xi]

            li = self.lower.get(xi)
            if li is not None and axi < li:
                for j, xj in nonbasic:
                    aij = self.tableau.get(i, j)
                    axj = self.assignment[xj]
                    if aij > 0 and (xj not in self.upper or axj < self.upper[xj]):
                        return i, j, li
                    if aij < 0 and (xj not in self.upper or axj > self.lower[xj]):
                        return i, j, li

                return False

            ui = self.upper.get(xi)
            if ui is not None and axi > ui:
                for j, xj in nonbasic:
                    aij = self.tableau.get(i, j)
                    axj = self.assignment[xj]
                    if aij < 0 and (xj not in self.upper or axj < self.upper[xj]):
                        return i, j, ui
                    if aij > 0 and (xj not in self.upper or axj > self.lower[xj]):
                        return i, j, ui

                return False

        return True

    def prepare(self) -> None:
        '''
        Prepare equations for solving.
        '''
        variables = self.vars()
        indices = {x: i for i, x in enumerate(variables)}
        n = len(variables)
        m = len(self.equations)

        self.n_basic = n
        self.assignment = (m + n) * [Number(0)]

        self.tableau = Tableau()
        lower = dict()
        upper = dict()
        for i, x in enumerate(self.equations):
            for y in x.lhs:
                self.tableau.set(i, indices[y.var], self.tableau.get(i, indices[y.var]) + y.co)

            if x.op == Operator.LessEqual:
                upper[n + i] = x.rhs
            elif x.op == Operator.GreaterEqual:
                lower[n + i] = x.rhs
            elif x.op == Operator.Equal:
                upper[n + i] = x.rhs
                lower[n + i] = x.rhs
        self.lower = lower
        self.upper = upper

        self.variables = list(range(0, m+n))

        self.n_pivots = 0

    def solve(self) -> Optional[List[Tuple[str, Number]]]:
        '''
        Solve the (previously prepared) problem.
        '''
        while True:
            p = self.select()
            if p is True:
                return [(var, self.assignment[self.variables[i]])
                        for i, var in enumerate(self.vars())]
            if p is False:
                return None

            assert isinstance(p, tuple)
            self.pivot(*p)

    def __str__(self) -> str:
        '''
        Print the equations in the problem.
        '''
        return '\n'.join(f'{x}' for x in self.equations)
*/